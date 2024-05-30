/*
 * This program is licensed granted by STATE UNIVERSITY OF CAMPINAS - UNICAMP ("University")
 * for use of MassCCS software ("the Software") through this website
 * https://github.com/cces-cepid/MassCCS (the "Website").
 *
 * By downloading the Software through the Website, you (the "License") are confirming that you agree
 * that your use of the Software is subject to the academic license terms.
 *
 * For more information about MassCCS please contact: 
 * skaf@unicamp.br (Munir S. Skaf)
 * guido@unicamp.br (Guido Araujo)
 * samuelcm@unicamp.br (Samuel Cajahuaringa)
 * danielzc@unicamp.br (Daniel L. Z. Caetano)
 * zanottol@unicamp.br (Leandro N. Zanotto)
 */

#include "headers/MoleculeTarget.h"

//MoleculeTarget::MoleculeTarget(string &filename, unsigned int gas_buffer_flag) {
MoleculeTarget::MoleculeTarget(string &filename, unsigned int gas_buffer_flag, string &user_ff, unsigned int user_ff_flag) {
this->filename = filename;
this->gas_buffer_flag = gas_buffer_flag;
this->user_ff = user_ff;
this->user_ff_flag = user_ff_flag;

// identify the extension file is .xyz or pqr
size_t lastindex = filename.find_last_of("."); 

extension = filename.substr(filename.find_last_of(".")+1);

if (user_ff_flag) readUserFF(user_ff);
else defaultFF();

printFF();

if (extension == "pqr") {
  readPQRfile(filename);
} else if (extension == "xyz") {
  readXYZfile(filename);
} else {
  cout << "only acceptable PQR or XYZ format" << endl; 
  exit (EXIT_FAILURE);
}

calculateCenterOfMass(rcm);

moveToCenterOfMass(rcm);

calculateMoleculeRadius();

setInertia();

print();

}

void MoleculeTarget::readPQRfile(string &filename) {
string sstring, recordName, AtomName,residueName;
ifstream infile;
ifstream coordinates;
string type, line;
stringstream ss;

/* Check the number of atoms */
int i = 0, serial, chainId;
infile.open (filename);
if (infile.is_open()){
  while(getline(infile,sstring)) // To get you all the lines.
  {
    istringstream sstream(sstring);
    sstream >> recordName;
    if (recordName == "ATOM"){
	    ++i;
    }
  }
  infile.close();
}

natoms = i;

id = new int[natoms];
x = new double[natoms];
y = new double[natoms];
z = new double[natoms];
q = new double[natoms];
m = new double[natoms];
eps = new double[natoms];
sig = new double[natoms];
atomName = new string[natoms];

// read the coordinates and types
string atomType;
double xi, yi, zi, mi, qi, epsi, sigi, ri;
vector<double> ret(3);

i = 0;
infile.open(filename);
if (infile.is_open()) {
  while(getline(infile,sstring)) 
  {
    istringstream sstream(sstring);
    sstream >> recordName >> serial >> AtomName >> residueName >> chainId >> xi >> yi >> zi >> qi >> ri;
    if (recordName == "ATOM") {
      x[i] = xi;
      y[i] = yi;
      z[i] = zi;
      q[i] = qi;
      id[i] = i;
      //atomName[i] = AtomName.substr(0,1);
      //cout << AtomName << "  " << atomName[i] << endl;
      //ret = defaultparameters(atomName[i], gas_buffer_flag);
      if (user_ff_flag) {
        atomName[i] = AtomName;
        ret = assignedParameter(atomName[i]);
      } else {
        atomName[i] = AtomName.substr(0,1);
        ret = assignedParameter(atomName[i]);
      }
      mi = ret[0];
      m[i] = mi;
      eps[i] = ret[1];
      sig[i] = ret[2]; 
      this->mass += mi; 
      this->Q += qi; 
      ++i;     
    }
  }
  infile.clear();
  infile.close();
}	

}

void MoleculeTarget::readXYZfile(string &filename) {

ifstream coordinates;
string type, line;
stringstream ss;

// read in the file
coordinates.open(filename);

// read the number of atoms (first line)
getline(coordinates, line);
ss.clear();

// read the number of atoms
ss.str(line);
if (!(ss >> natoms)) {
  perror("Error reading xyz file");
  throw std::invalid_argument("Error reading xyz file");
}

id = new int[natoms];
x = new double[natoms];
y = new double[natoms];
z = new double[natoms];
q = new double[natoms];
m = new double[natoms];
eps = new double[natoms];
sig = new double[natoms];
atomName = new string[natoms];

// skip the second line (comment)
getline(coordinates, line);

// read the coordinates and types
string atomType;
double xi, yi, zi, mi, qi, epsi, sigi;
vector<double> ret(3);

for (unsigned int i = 0; i < natoms; i++) {
  getline(coordinates, line);
  ss.clear();
  ss.str(line);
  /*
  if (ss >> atomType >> xi >> yi >> zi) {
    id[i] = i;
    atomName[i] = atomType;
    ret = defaultparameters(atomType, gas_buffer_flag);  
    x[i] = xi;
    y[i] = yi;
    z[i] = zi;
    q[i] = 0.0;
    mi = ret[0];
    m[i] = mi;
    eps[i] = ret[1];
    sig[i] = ret[2]; 
    this->mass += mi; 
    this->Q += 0.0; */
  if (ss >> atomType >> xi >> yi >> zi >> qi) {
    id[i] = i;
    atomName[i] = atomType;
    //ret = defaultparameters(atomType, gas_buffer_flag);  
    ret = assignedParameter(atomName[i]);
    x[i] = xi;
    y[i] = yi;
    z[i] = zi;
    q[i] = qi;
    mi = ret[0];
    m[i] = mi;
    eps[i] = ret[1];
    sig[i] = ret[2]; 
    this->mass += mi; 
    this->Q += qi; 
  } else {
    throw invalid_argument("Error reading xyz file");
  }
}

coordinates.close();
}

void MoleculeTarget::readUserFF(string &user_ff) {
ifstream userfile;
string type, line;
stringstream ss;

// read in the file
userfile.open(user_ff);

// read the number of atoms (first line)
getline(userfile, line);
ss.clear();

// read the number of parameters
ss.str(line);
if (!(ss >> nparameters)) {
  perror("Error reading xyz file");
  throw std::invalid_argument("Error reading xyz file");
}

user_atomName = new string[nparameters];
user_m = new double[nparameters];
user_eps = new double[nparameters];
user_sig = new double[nparameters];


// skip the second line (comment)
getline(userfile, line);

// read the coordinates and types
string atomType;
double mi, epsi, sigi;

for (unsigned int i = 0; i < nparameters; i++) {
  getline(userfile, line);
  ss.clear();
  ss.str(line);

  if (ss >> atomType >> mi >> epsi >> sigi) {
    user_atomName[i] = atomType;
    user_m[i] = mi;
    user_eps[i] = epsi;
    user_sig[i] = sigi;
  } else {
    throw invalid_argument("Error reading force field parameters file");
  }
}

userfile.close();

}

void MoleculeTarget::defaultFF() {
double m, eps, sig;
string carbon = "C";
string nitrogen = "N";
string hydrogen = "H";
string oxygen = "O";
string sulfur = "S";
string phosphorus = "P";

// for hellium
if (gas_buffer_flag == 1) {
  nparameters = 5;
  user_atomName = new string[nparameters];
  user_m = new double[nparameters];
  user_eps = new double[nparameters];
  user_sig = new double[nparameters];

  user_atomName[0] = carbon;
  user_m[0] = 12.011;
  user_eps[0] = 0.0309;
  user_sig[0] = 3.043;

  user_atomName[1] = nitrogen;
  user_m[1] = 14.007;
  user_eps[1] = 0.0309;
  user_sig[1] = 3.043;

  user_atomName[2] = hydrogen;
  user_m[2] = 1.008;
  user_eps[2] = 0.0150;
  user_sig[2] = 2.38;

  user_atomName[3] = oxygen;
  user_m[3] = 15.999;
  user_eps[3] = 0.0309;
  user_sig[3] = 3.043;

  user_atomName[4] = sulfur;
  user_m[4] = 32.06;
  user_eps[4] = 0.0311;
  user_sig[4] = 3.5;
// for nitrogen  
} else if (gas_buffer_flag == 2) {
  nparameters = 6;
  user_atomName = new string[nparameters];
  user_m = new double[nparameters];
  user_eps = new double[nparameters];
  user_sig = new double[nparameters];

  user_atomName[0] = carbon;
  user_m[0] = 12.011;
  user_eps[0] = 0.0824736;
  user_sig[0] = 3.2255;

  user_atomName[1] = nitrogen;
  user_m[1] = 14.007;
  user_eps[1] = 0.0758527;
  user_sig[1] = 3.5719;

  user_atomName[2] = hydrogen;
  user_m[2] = 1.008;
  user_eps[2] = 0.0362711;
  user_sig[2] = 1.8986;

  user_atomName[3] = oxygen;
  user_m[3] = 15.999;
  user_eps[3] = 0.062323;
  user_sig[3] = 3.0750;

  user_atomName[4] = sulfur;
  user_m[4] = 32.06;
  user_eps[4] = 0.138032;
  user_sig[4] = 3.4237;

  user_atomName[5] = phosphorus;
  user_m[5] = 30.9738;
  user_eps[5] = 0.145372;
  user_sig[5] = 3.47;
}
// for carbon dioxide
// force field parameter from charmm file par_all27_prot_na.prm
/*} else if (gas_buffer_flag == 3) {
  if (chemical == carbon) {
    m = 12.011;
    eps = 0.11;
    sig = 3.564;
  } else if (chemical == nitrogen) {
    m = 14.007;
    eps = 0.200;
    sig = 3.296;
  } else if (chemical == hydrogen) {
    m = 1.008;
    eps = 0.046;
    sig = 0.400;
  } else if (chemical == oxygen) {
    m = 15.999;
    eps = 0.12;
    sig = 3.029;
  } else if (chemical == sulfur) {
    m = 32.06;
    eps = 0.45;
    sig = 3.564;
  } else if (chemical == phosphorus) {
    m = 30.9738;
    eps = 0.585;
    sig = 3.831;
  }
} */
}

vector<double> MoleculeTarget::assignedParameter(string chemical) {

vector<double> ret(3);

for (int i = 0; i < nparameters; i++) {
  if (chemical == user_atomName[i]) {
    ret[0] = user_m[i];
    ret[1] = user_eps[i];
    ret[2] = user_sig[i];
    return ret;
  }
}

if (user_ff_flag) cout << "Atom type not found in the file user force field" << endl;
else cout << "Atom type not found in the default data base " << endl;
exit (EXIT_FAILURE);

}

void MoleculeTarget::printFF() {

cout << "*********************************************************" << endl;
cout << "Force Field parameters: " << endl;
cout << "*********************************************************" << endl;
cout << "Symbol  mass (amu)  epislon (kcal/mol)  sigma(Angstroms) " << endl;
for (int i = 0; i < nparameters; i++) {
  cout << user_atomName[i] << "   " << user_m[i] << "   " << user_eps[i] << "   " << user_sig[i] << endl;
}

}

/*vector<double> MoleculeTarget::defaultparameters(string chemical, unsigned int gas_buffer_flag) {

vector<double> ret(3);

double m, eps, sig;

string carbon = "C";
string nitrogen = "N";
string hydrogen = "H";
string oxygen = "O";
string sulfur = "S";
string phosphorus = "P";

// for hellium
if (gas_buffer_flag == 1) {
  if (chemical == carbon) {
    m = 12.011;
    eps = 0.0309;
    sig = 3.043;
  } else if (chemical == nitrogen) {
    m = 14.007;
    eps = 0.0309;
    sig = 3.043;
  } else if (chemical == hydrogen) {
    m = 1.008;
    eps = 0.0150;
    sig = 2.38;
  } else if (chemical == oxygen) {
    m = 15.999;
    eps = 0.0309;
    sig = 3.043;
  } else if (chemical == sulfur) {
    m = 32.06;
    eps = 0.0311;
    sig = 3.5;
  }	
// for nitrogen  
} else if (gas_buffer_flag == 2) {
  if (chemical == carbon) {
    m = 12.011;
    eps = 0.0824736;
    sig = 3.2255;
  } else if (chemical == nitrogen) {
    m = 14.007;
    eps = 0.0758527;
    sig = 3.5719;
  } else if (chemical == hydrogen) {
    m = 1.008;
    eps = 0.0362711;
    sig = 1.8986;
  } else if (chemical == oxygen) {
    m = 15.999;
    eps = 0.062323;
    sig = 3.0750;
  } else if (chemical == sulfur) {
    m = 32.06;
    eps = 0.138032;
    sig = 3.4237;
  } else if (chemical == phosphorus) {
    m = 30.9738;
    eps = 0.145372;
    sig = 3.47;
  }
// for carbon dioxide
// force field parameter from charmm file par_all27_prot_na.prm
} else if (gas_buffer_flag == 3) {
  if (chemical == carbon) {
    m = 12.011;
    eps = 0.11;
    sig = 3.564;
  } else if (chemical == nitrogen) {
    m = 14.007;
    eps = 0.200;
    sig = 3.296;
  } else if (chemical == hydrogen) {
    m = 1.008;
    eps = 0.046;
    sig = 0.400;
  } else if (chemical == oxygen) {
    m = 15.999;
    eps = 0.12;
    sig = 3.029;
  } else if (chemical == sulfur) {
    m = 32.06;
    eps = 0.45;
    sig = 3.564;
  } else if (chemical == phosphorus) {
    m = 30.9738;
    eps = 0.585;
    sig = 3.831;
  }
} 

ret[0] = m;
ret[1] = eps;
ret[2] = sig;

return ret;
}*/	

void MoleculeTarget::calculateCenterOfMass(double (&rcm)[3]) {

rcm[0] = 0.0;
rcm[1] = 0.0;
rcm[2] = 0.0;

for (unsigned int i = 0; i < natoms; i++) {
  rcm[0] += m[i]*x[i];
  rcm[1] += m[i]*y[i];
  rcm[2] += m[i]*z[i];
}
rcm[0] /= mass;
rcm[1] /= mass;
rcm[2] /= mass;
}

void MoleculeTarget::moveToCenterOfMass(double rcm[3]) {

for (unsigned int i = 0; i < natoms; i++) {
  x[i] -= rcm[0];
  y[i] -= rcm[1];
  z[i] -= rcm[2];
}

}

// orientate the molecule along Z-axis
void MoleculeTarget::orientateMolecule() {
double theta, phi, gamma;
double rmax, r;
int atom_id;
double xi, yi, zi;
double pos[3];
rmax = 0.0;

// angle theta and phi of more distance particle of molecule
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   yi = y[i];
   zi = z[i];
   r = sqrt(xi*xi + yi*yi + zi*zi);

   if (r > rmax) {
     atom_id = id[i];
     rmax = r;
   }
}

xi = x[atom_id];
yi = y[atom_id];
zi = z[atom_id];

if (xi != 0.0 & yi != 0.0) theta = atan2(yi,xi);

phi = acos(zi/rmax);

//printf("rmax: %f theta: %f phi: %f id: %i\n",rmax,theta*180/M_PI,phi*180/M_PI,id);

// rotation around z axis
theta *= -1.0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   yi = y[i];
   x[i] = cos(theta)*xi - sin(theta)*yi; 
   y[i] = sin(theta)*xi + cos(theta)*yi;
}

// rotation around y axis
//phi = M_PI/2.0 - phi;
phi *= -1.0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   zi = z[i];
   x[i] = sin(phi)*zi + cos(phi)*xi;
   z[i] = cos(phi)*zi - sin(phi)*xi;   
}

// angle gamma of more distance particle of molecule in xy plane
rmax = 0.0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   yi = y[i];
   r = sqrt(xi*xi + yi*yi);

   if (r > rmax) {
     atom_id = id[i];
     rmax = r;
   }
}

xi = x[atom_id];
yi = y[atom_id];

gamma = atan2(yi,xi);
// rotation around z axis
gamma *= -1.0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   yi = y[i];
   x[i] = cos(gamma)*xi - sin(gamma)*yi;
   y[i] = sin(gamma)*xi + cos(gamma)*yi;
}


}

// inertia tensor and rotate molecule along principle axes
void MoleculeTarget::setInertia() {
int i, a, b, maxcoord;
std::vector<double> temp1(3), temp2(3), temp3(3);
double dotProd, temp, maxMag, phi, theta, gamma;
double mi;
double r[3],r2;
double xi, yi, zi;
std::vector<double> position(3, 0.0);

//initialize the elements of the array
for (a = 0; a < 3; a ++)
   for (b = 0; b < 3; b ++)
      inertia[a][b] = 0.0;

//perform the calculation of the inertia matrix
for (unsigned int i = 0; i < natoms; i++) {
   r[0] = x[i];
   r[1] = y[i];
   r[2] = z[i];
   mi = m[i];
   r2 = r[0]*r[0] + r[1]*r[1] + r[2]*r[2];
   for (a = 0; a < 3; a++) {
      for (b = 0; b < 3; b++) {
         inertia[a][b] += mi*(r2*delta(a, b) - r[a]*r[b]);
      }
   }
}

//diagonalize the inertia tensor
getEigenvalues(inertia, inertiaValues);
getEigenvectors(inertia, inertiaValues, inertiaVectors);

temp1[0] = inertiaVectors[0][0];
temp1[1] = inertiaVectors[0][1];
temp1[2] = inertiaVectors[0][2];
temp2[0] = inertiaVectors[1][0];
temp2[1] = inertiaVectors[1][1];
temp2[2] = inertiaVectors[1][2];
temp3[0] = inertiaVectors[2][0];
temp3[1] = inertiaVectors[2][1];
temp3[2] = inertiaVectors[2][2];

dotProd = sqrt(pow(Math::dotProduct(temp1,temp2),2.0));
dotProd = max(sqrt(pow(Math::dotProduct(temp1,temp3),2.0)),dotProd);
dotProd = max(sqrt(pow(Math::dotProduct(temp2,temp3),2.0)),dotProd);

if (dotProd > 1e-6) {
  orientateMolecule();
} else {
  for (unsigned int i = 0; i < natoms; i++) {
     xi = x[i];
     yi = y[i];
     zi = z[i];
     x[i] = xi*inertiaVectors[0][0] + yi*inertiaVectors[0][1] + zi*inertiaVectors[0][2];
     y[i] = xi*inertiaVectors[1][0] + yi*inertiaVectors[1][1] + zi*inertiaVectors[1][2];
     z[i] = xi*inertiaVectors[2][0] + yi*inertiaVectors[2][1] + zi*inertiaVectors[2][2];
  }
}

//find direction of longest extent
maxX = 0; maxY = 0; maxZ = 0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   if (abs(xi) > abs(maxX)) maxX = abs(xi);
   yi = y[i];
   if (abs(yi) > abs(maxY)) maxY = abs(yi);
   zi = z[i];
   if (abs(zi) > abs(maxZ)) maxZ = abs(zi);
}

//if longest extent is along x-axis,
//rotate about y-axis -pi/2
if (maxX > maxY && maxX > maxZ) {
  for (unsigned int i = 0; i < natoms; i++) {
     xi = x[i];
     zi = z[i];
     x[i] = -zi;
     z[i] = xi;
  }
} else if (maxY > maxX && maxY > maxZ) {
  for (unsigned int i = 0; i < natoms; i++) {
     yi = y[i];
     zi = z[i];
     y[i] = zi;
     z[i] = -yi;
  }
}

//reset maximum extent values
maxX = 0; maxY = 0; maxZ = 0;
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   if (abs(xi) > abs(maxX)) maxX = xi;
   yi = y[i];
   if (abs(yi) > abs(maxY)) maxY = yi;
   zi = z[i];
   if (abs(zi) > abs(maxZ)) maxZ = zi;
}

}

/* This function finds the exact eigenvalues for a given 3x3 matrix A
 * The characteristic equation is solved exactly to find the eigenvalues,
 * and then these are used to find the eigenvectors of the vector.
 * A 3x3 matrix of orthogonal eigenvectors are returned.
 */
void MoleculeTarget::getEigenvalues(double A[3][3], double (&eigenvalues)[3]) {
//double eigenvalues[3];
//eigenvalues = new double[3];
double a0, a1, a2;
double p, q, R, Q, D, a, b, phase, mag;

//roots of the characteristic equation
a0 = A[0][0]*A[1][1]*A[2][2] -
     A[0][0]*A[1][2]*A[2][1] -
     A[1][1]*A[0][2]*A[2][0] -
     A[2][2]*A[0][1]*A[1][0] +
     A[0][1]*A[1][2]*A[2][0] +
     A[0][2]*A[2][1]*A[1][0];

a1 = A[0][1]*A[1][0] + A[0][2]*A[2][0] + A[1][2]*A[2][1] - A[0][0]*A[1][1] - A[1][1]*A[2][2] -
     A[2][2]*A[0][0];

a2 = A[0][0] + A[1][1] + A[2][2];
a0 = -a0; a1 = -a1; a2 = -a2;

//solve the cubic equation for 3 real roots
p = (3*a1 - a2*a2)/3;
q = (9*a1*a2 - 27*a0 - 2*a2*a2*a2)/27;
Q = p/3; R = q/2; D = Q*Q*Q + R*R;

if (D < 0) {
  a = R; b = sqrt(-D);
} else {
  a = R + sqrt(D);
  b = 0;
}

phase = atan2(b, a);
mag = pow(a*a + b*b,1.0/6.0);

eigenvalues[0] = -(a2/3) + mag*2*cos(phase/3.0);
eigenvalues[1] = -(a2/3) - mag*(cos(phase/3.0) + sqrt(3.0)*sin(phase/3.0));
eigenvalues[2] = -(a2/3) - mag*(cos(phase/3.0) - sqrt(3.0)*sin(phase/3.0));
}

void MoleculeTarget::getEigenvectors(double A[3][3], double lambda[3], double (&eigenvectors)[3][3]) {
//double eigenvectors[3][3];
double det, x, y, z, normFactor;
double temp[3][3];

det = lambda[0] * lambda[0] - lambda[0] * A[1][1] - A[1][2] * A[1][2]
    - lambda[0] * A[2][2] + A[1][1] * A[2][2];
y = (lambda[0] * A[1][0] - A[2][2] * A[1][0] + A[1][2] * A[2][0]) / det;
z = (A[2][1] * A[1][0] + lambda[0] * A[2][0] - A[1][1] * A[2][0]) / det;
normFactor = sqrt(1 + y*y + z*z);
eigenvectors[0][0] = 1.0 / normFactor;
eigenvectors[0][1] = y / normFactor;
eigenvectors[0][2] = z / normFactor;

det = lambda[1] * lambda[1] - lambda[1] * A[0][0] - A[0][2] * A[0][2]
    - lambda[1] * A[2][2] + A[0][0] * A[2][2];
x = (lambda[1] * A[0][1] + A[0][2] * A[2][1] - A[0][1] * A[2][2]) / det;
z = (A[0][1] * A[0][2] + (lambda[1] - A[0][0])*A[2][1]) / det;
normFactor = sqrt(1 + x*x + z*z);
eigenvectors[1][0] = x / normFactor;
eigenvectors[1][1] = 1 / normFactor;
eigenvectors[1][2] = z / normFactor;

eigenvectors[2][0] = eigenvectors[0][1] * eigenvectors[1][2] - eigenvectors[0][2] * eigenvectors[1][1];
eigenvectors[2][1] = eigenvectors[0][2] * eigenvectors[1][0] - eigenvectors[0][0] * eigenvectors[1][2];
eigenvectors[2][2] = eigenvectors[0][0] * eigenvectors[1][1] - eigenvectors[0][1] * eigenvectors[1][0];
}

double MoleculeTarget::delta(int a, int b) {
    if(a == b)
        return 1.0;
    else
        return 0.0;
}

void MoleculeTarget::print() {

cout << "*********************************************************" << endl;
cout << "MOLECULE:: orientation around the inertia principal axis " << endl;
cout << "*********************************************************" << endl;
cout << "Inertia matrix a.u. Ang^2: " << endl;
cout << "{  " << inertia[0][0] << "  " << inertia[0][1] << "  " << inertia[0][2] << "  }" << endl;
cout << "{  " << inertia[1][0] << "  " << inertia[1][1] << "  " << inertia[1][2] << "  }" << endl;
cout << "{  " << inertia[2][0] << "  " << inertia[2][1] << "  " << inertia[2][2] << "  }" << endl;

cout << "Rotation matrix applied to molecule: " << endl;
cout << "{  " << inertiaVectors[0][0] << "  " << inertiaVectors[0][1] << "  " << inertiaVectors[0][2] << "  }" << endl;
cout << "{  " << inertiaVectors[1][0] << "  " << inertiaVectors[1][1] << "  " << inertiaVectors[1][2] << "  }" << endl;
cout << "{  " << inertiaVectors[2][0] << "  " << inertiaVectors[2][1] << "  " << inertiaVectors[2][2] << "  }" << endl;

cout << "Molecule radius: " << moleculeRadius << " Ang " << endl;
}

void MoleculeTarget::calculateMoleculeRadius() {
double theta, phi, gamma;
double rmax, r;
int atom_id;
double xi, yi, zi;
double pos[3];
rmax = 0.0;

// angle theta and phi of more distance particle of molecule
for (unsigned int i = 0; i < natoms; i++) {
   xi = x[i];
   yi = y[i];
   zi = z[i];
   r = sqrt(xi*xi + yi*yi + zi*zi);

   if (r > rmax) {
     atom_id = id[i];
     rmax = r;
   }
}

xi = x[atom_id];
yi = y[atom_id];
zi = z[atom_id];

moleculeRadius = sqrt(xi*xi + yi*yi + zi*zi);
}

