/*************************************************************************************

    Grid physics library, www.github.com/paboyle/Grid

    Source file: ./tests/Test_wilson_force.cc

    Copyright (C) 2015

Author: Peter Boyle <paboyle@ph.ed.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    See the full license in the file "LICENSE" in the top level distribution directory
    *************************************************************************************/
/*  END LEGAL */
#include <Grid/Grid.h>

using namespace std;
using namespace Grid;
using namespace Grid::QCD;

int main(int argc, char **argv)
{
  Grid_init(&argc, &argv);

  std::vector<int> latt_size = GridDefaultLatt();
  std::vector<int> simd_layout = GridDefaultSimd(Nd, vComplex::Nsimd());
  std::vector<int> mpi_layout = GridDefaultMpi();

  GridCartesian Grid(latt_size, simd_layout, mpi_layout);
  GridRedBlackCartesian RBGrid(&Grid);

  int threads = GridThread::GetThreads();
  std::cout << GridLogMessage << "Grid is setup to use " << threads << " threads" << std::endl;

  std::vector<int> seeds({1, 2, 30, 50});

  GridParallelRNG pRNG(&Grid);
  
  std::vector<int> vrand(4);
  std::srand(std::time(0));
  std::generate(vrand.begin(), vrand.end(), std::rand);
  std::cout << GridLogMessage << vrand << std::endl;
  pRNG.SeedFixedIntegers(vrand);
  
  //pRNG.SeedFixedIntegers(seeds);

  LatticeFermion phi(&Grid);
  gaussian(pRNG, phi);
  LatticeFermion Mphi(&Grid);
  LatticeFermion MphiPrime(&Grid);

  LatticeGaugeField U(&Grid);

/*
  std::vector<int> x(4); // 4d fermions
  std::vector<int> gd = Grid.GlobalDimensions();
  Grid::QCD::SpinColourVector F;
  Grid::Complex c;

  phi = zero;
  for (x[0] = 0; x[0] < 1; x[0]++)
  {
    for (x[1] = 0; x[1] < 1; x[1]++)
    {
      for (x[2] = 0; x[2] < 1; x[2]++)
      {
        for (x[3] = 0; x[3] < 1; x[3]++)
        {
          for (int sp = 0; sp < 4; sp++)
          {
            for (int j = 0; j < 3; j++) // colours
            {
              F()(sp)(j) = Grid::Complex(0.0,0.0);
              if (((sp == 0) && (j==0)))
              {
                c = Grid::Complex(1.0, 0.0);
                F()(sp)(j) = c;
              }
            }
          }
          Grid::pokeSite(F, phi, x);

        }
      }
    }
  }
*/

  std::vector<int> site = {0, 0, 0, 0};
  SU3::HotConfiguration(pRNG, U);
  //SU3::ColdConfiguration(pRNG, U);


  ////////////////////////////////////
  // Unmodified matrix element
  ////////////////////////////////////
  RealD mass = -4.0; //kills the diagonal term
  Real csw = 1.0;
  WilsonCloverFermionR Dw(U, Grid, RBGrid, mass, csw);
  Dw.ImportGauge(U);
  Dw.M(phi, Mphi);
  ComplexD S = innerProduct(Mphi, Mphi); // Action : pdag MdagM p

  // get the deriv of phidag MdagM phi with respect to "U"
  LatticeGaugeField UdSdU(&Grid);
  LatticeGaugeField tmp(&Grid);

  ////////////////////////////////////////////
  Dw.MDeriv(tmp, Mphi, phi, DaggerNo);
  UdSdU = tmp;
  Dw.MDeriv(tmp, phi, Mphi, DaggerYes);
  UdSdU += tmp;
  /////////////////////////////////////////////

  // Take the traceless antihermitian component
  //UdSdU = Ta(UdSdU);

  //std::cout << UdSdU << std::endl;
  //SU3::LatticeAlgebraVector hforce(&Grid);
  LatticeColourMatrix mommu(&Grid);
  //mommu = PeekIndex<LorentzIndex>(UdSdU, 0);
  //SU3::projectOnAlgebra(hforce, mommu);
  //std::cout << hforce << std::endl;

  ////////////////////////////////////
  // Modify the gauge field a little
  ////////////////////////////////////
  RealD dt = 0.0001;
  RealD Hmom = 0.0;
  RealD Hmomprime = 0.0;
  RealD Hmompp = 0.0;
  LatticeColourMatrix forcemu(&Grid);
  LatticeGaugeField mom(&Grid);
  LatticeGaugeField Uprime(&Grid);

  
  for (int mu = 0; mu < Nd; mu++) {
    // Traceless antihermitian momentum; gaussian in lie alg
    SU3::GaussianFundamentalLieAlgebraMatrix(pRNG, mommu);
    Hmom -= real(sum(trace(mommu * mommu)));
    PokeIndex<LorentzIndex>(mom, mommu, mu);
  }
  /*
  SU3::AlgebraVector h;
  SU3::LatticeAlgebraVector hl(&Grid);
  h()()(0) = 1.0;
  hl = zero;
  pokeSite(h, hl, site);
  SU3::FundamentalLieAlgebraMatrix(hl, mommu);
  mom = zero;
  PokeIndex<LorentzIndex>(mom, mommu, 0);
  Hmom -= real(sum(trace(mommu * mommu)));
  */

  /*
  parallel_for(int ss=0;ss<mom._grid->oSites();ss++){
    for (int mu = 0; mu < Nd; mu++)
      Uprime[ss]._internal[mu] = ProjectOnGroup(Exponentiate(mom[ss]._internal[mu], dt, 12) * U[ss]._internal[mu]);
  }
*/

  for (int mu = 0; mu < Nd; mu++)
  {
    parallel_for(auto i = mom.begin(); i < mom.end(); i++)
    {
      Uprime[i](mu) = U[i](mu);
      Uprime[i](mu) += mom[i](mu) * U[i](mu) * dt;
      Uprime[i](mu) += mom[i](mu) * mom[i](mu) * U[i](mu) * (dt * dt / 2.0);
      Uprime[i](mu) += mom[i](mu) * mom[i](mu) * mom[i](mu) * U[i](mu) * (dt * dt * dt / 6.0);
      Uprime[i](mu) += mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * U[i](mu) * (dt * dt * dt * dt / 24.0);
      Uprime[i](mu) += mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * U[i](mu) * (dt * dt * dt * dt * dt / 120.0);
      Uprime[i](mu) += mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * mom[i](mu) * U[i](mu) * (dt * dt * dt * dt * dt * dt / 720.0);
    }
  }

  std::cout << GridLogMessage << "Initial mom hamiltonian is " << Hmom << std::endl;

  // New action
  LatticeGaugeField diff(&Grid);
  diff = Uprime - U;
  //std::cout << "Diff:" << diff << std::endl;
  Dw.ImportGauge(Uprime);
  Dw.M(phi, MphiPrime);
  LatticeFermion DiffFermion(&Grid);
  DiffFermion = MphiPrime - Mphi;
  //std::cout << "DiffFermion:" << DiffFermion << std::endl;
  //std::cout << "Mphi:" << Mphi << std::endl;
  //std::cout << "MphiPrime:" << MphiPrime << std::endl;

  ComplexD Sprime = innerProduct(MphiPrime, MphiPrime);

  //////////////////////////////////////////////
  // Use derivative to estimate dS
  //////////////////////////////////////////////

  ///////////////////////////////////////////////////////
  std::cout << GridLogMessage << "Antihermiticity tests - 1 " << std::endl;
  for (int mu = 0; mu < Nd; mu++)
  {
    mommu = PeekIndex<LorentzIndex>(mom, mu);
    std::cout << GridLogMessage << " Mommu  " << norm2(mommu) << std::endl;
    mommu = mommu + adj(mommu);
    std::cout << GridLogMessage << " Test: Mommu + Mommudag " << norm2(mommu) << std::endl;
    mommu = PeekIndex<LorentzIndex>(UdSdU, mu);
    std::cout << GridLogMessage << " dsdumu  " << norm2(mommu) << std::endl;
    mommu = mommu + adj(mommu);
    std::cout << GridLogMessage << " Test: dsdumu + dag  " << norm2(mommu) << std::endl;
    std::cout << "" << std::endl;
  }
  ////////////////////////////////////////////////////////

  LatticeComplex dS(&Grid);
  dS = zero;
  LatticeComplex dSmom(&Grid);
  dSmom = zero;
  LatticeComplex dSmom2(&Grid);
  dSmom2 = zero;


  for (int mu = 0; mu < Nd; mu++)
  {
    mommu = PeekIndex<LorentzIndex>(UdSdU, mu); // P_mu =
    mommu = Ta(mommu) * 2.0;                    // Mom = (P_mu - P_mu^dag) - trace(P_mu - P_mu^dag)
    PokeIndex<LorentzIndex>(UdSdU, mommu, mu);  // UdSdU_mu = Mom
  }

  std::cout << GridLogMessage << "Antihermiticity tests - 2 " << std::endl;
  for (int mu = 0; mu < Nd; mu++)
  {
    mommu = PeekIndex<LorentzIndex>(mom, mu);
    std::cout << GridLogMessage << " Mommu  " << norm2(mommu) << std::endl;
    mommu = mommu + adj(mommu);
    std::cout << GridLogMessage << " Mommu + Mommudag " << norm2(mommu) << std::endl;
    mommu = PeekIndex<LorentzIndex>(UdSdU, mu);
    std::cout << GridLogMessage << " dsdumu  " << norm2(mommu) << std::endl;
    mommu = mommu + adj(mommu);
    std::cout << GridLogMessage << " dsdumu + dag  " << norm2(mommu) << std::endl;
    std::cout << "" << std::endl;
  }
  /////////////////////////////////////////////////////

  for (int mu = 0; mu < Nd; mu++)
  {
    forcemu = PeekIndex<LorentzIndex>(UdSdU, mu);
    mommu = PeekIndex<LorentzIndex>(mom, mu);

    // Update PF action density
    dS = dS + trace(mommu * forcemu) * dt;

    dSmom = dSmom - trace(mommu * forcemu) * dt;
    dSmom2 = dSmom2 - trace(forcemu * forcemu) * (0.25 * dt * dt);

    // Update mom action density
    mommu = mommu + forcemu * (dt * 0.5);

    Hmomprime -= real(sum(trace(mommu * mommu)));
  }

  ComplexD dSpred = sum(dS);
  ComplexD dSm = sum(dSmom);
  ComplexD dSm2 = sum(dSmom2);

  std::cout << GridLogMessage << "Initial mom hamiltonian is " << Hmom << std::endl;
  std::cout << GridLogMessage << "Final   mom hamiltonian is " << Hmomprime << std::endl;
  std::cout << GridLogMessage << "Delta   mom hamiltonian is " << Hmomprime - Hmom << std::endl;

  std::cout << GridLogMessage << " S      " << S << std::endl;
  std::cout << GridLogMessage << " Sprime " << Sprime << std::endl;
  std::cout << GridLogMessage << "dS      " << Sprime - S << std::endl;
  std::cout << GridLogMessage << "predict dS    " << dSpred << std::endl;
  std::cout << GridLogMessage << "dSm " << dSm << std::endl;
  std::cout << GridLogMessage << "dSm2" << dSm2 << std::endl;

  std::cout << GridLogMessage << "Total dS    " << Hmomprime - Hmom + Sprime - S << std::endl;

  assert(fabs(real(Sprime - S - dSpred)) < 1.0);

  std::cout << GridLogMessage << "Done" << std::endl;
  Grid_finalize();
}