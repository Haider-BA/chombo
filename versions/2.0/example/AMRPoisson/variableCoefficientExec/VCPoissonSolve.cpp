#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

#include <iostream>
#include "ParmParse.H"
#include "LoadBalance.H"
#include "LevelData.H"
#include "FArrayBox.H"
#include "BRMeshRefine.H"
#include "VCAMRPoissonOp.H"
#include "FABView.H"
#include "DebugDump.H"
#include "VClocalFuncs.H"
#include "MultilevelLinearOp.H"
#include "BiCGStabSolver.H"
#include "AMRIO.H"
#include "UsingNamespace.H"

using std::cerr;

void
setAlpha(LevelData<FArrayBox>& a_alpha, 
         const VCPoissonParameters& a_params,
         const RealVect& a_dx)
{
  RealVect pos;
  int num;
  DataIterator dit = a_alpha.dataIterator();
  for (dit.begin(); dit.ok(); ++dit)
    {
      FArrayBox& alpha = a_alpha[dit];
      ForAllXBNN(Real, alpha, alpha.box(), 0, alpha.nComp());
      {
        num = nR;
        D_TERM(pos[0]=a_dx[0]*(iR+0.5);, 
               pos[1]=a_dx[1]*(jR+0.5);, 
               pos[2]=a_dx[2]*(kR+0.5));
        alphaR = a_params.alphaFactor*pos[0];
        // constant-coefficient 
        //alphaR = a_params.alphaFactor;
      }EndFor;
    } // end loop over grids
}


void
setBeta(LevelData<FluxBox>& a_beta,
        const VCPoissonParameters& a_params,
        const RealVect& a_dx)
{
  DataIterator dit = a_beta.dataIterator();
  for (dit.begin(); dit.ok(); ++dit)
    {
      FluxBox& thisBeta = a_beta[dit];
      for (int dir=0; dir<SpaceDim; dir++)
        {
          FArrayBox& dirFlux = thisBeta[dir];
          const Box& dirBox = dirFlux.box();
          // this sets up a vector which is 0 in the dir
          // direct and 0.5 in the other (cell-centered) directions
          RealVect offsets = BASISREALV(dir);
          RealVect pos;
          offsets -= RealVect::Unit;
          offsets *= -0.5;
          int n;
          ForAllXBNN(Real, dirFlux, dirBox, 0, dirFlux.nComp())
            {
              n = nR;
              D_TERM(pos[0] = a_dx[0]*(iR+offsets[0]);,
                     pos[1] = a_dx[1]*(jR+offsets[1]);,
                     pos[2] = a_dx[2]*(kR+offsets[2]));
              dirFluxR = a_params.betaFactor*D_TERM(pos[0], +pos[1], +pos[2]);
              // constant-coefficient
              //dirFluxR = a_params.betaFactor;
            }EndFor
        } // end loop over directions
    }
}



/******/
void poissonSolve(Vector<LevelData<FArrayBox>* >& a_phi,
                  Vector<LevelData<FArrayBox>* >& a_rhs,
                  const Vector< DisjointBoxLayout >&   a_grids, 
                  const VCPoissonParameters&             a_params)
{
  ParmParse pp;

  int nlevels = a_params.numLevels;
  a_phi.resize(nlevels);
  a_rhs.resize(nlevels);
  Vector<RefCountedPtr<LevelData<FArrayBox> > > alpha(nlevels);
  Vector<RefCountedPtr<LevelData<FluxBox> > > beta(nlevels);
  Vector<ProblemDomain> vectDomains(nlevels);
  Vector<RealVect> vectDx(nlevels);

  RealVect dxLev = RealVect::Unit;
  dxLev *=a_params.coarsestDx;
  ProblemDomain domLev(a_params.coarsestDomain);
  for(int ilev = 0; ilev < nlevels; ilev++)
    {
      a_rhs[ilev] = new LevelData<FArrayBox>(a_grids[ilev], 1,IntVect::Zero);
      a_phi[ilev] = new LevelData<FArrayBox>(a_grids[ilev], 1,IntVect::Unit);
      alpha[ilev] = RefCountedPtr<LevelData<FArrayBox> >(new LevelData<FArrayBox>(a_grids[ilev], 1, IntVect::Zero));
      beta[ilev] = RefCountedPtr<LevelData<FluxBox> >(new LevelData<FluxBox>(a_grids[ilev], 1, IntVect::Zero));
      vectDomains[ilev] = domLev;
      vectDx[ilev] = dxLev;

      setAlpha(*alpha[ilev], a_params, dxLev);
      setBeta(*beta[ilev], a_params, dxLev);

      for(DataIterator dit = a_grids[ilev].dataIterator(); dit.ok(); ++dit)
        {
          (*a_phi[ilev])[dit()].setVal(0.);
        }

      setRHS (*a_rhs[ilev], dxLev, a_params);

      //prepare dx, domain for next level
      dxLev /=      a_params.refRatio[ilev];
      domLev.refine(a_params.refRatio[ilev]);
   }

  // set up solver
  RefCountedPtr<AMRLevelOpFactory<LevelData<FArrayBox> > > opFactory
    = RefCountedPtr<AMRLevelOpFactory<LevelData<FArrayBox> > >(defineOperatorFactory( a_grids, vectDomains,
                                                                                      alpha, beta, a_params));

  int lBase = 0;
  MultilevelLinearOp<FArrayBox> mlOp;
  int numMGIter = 1;
  pp.query("numMGIterations", numMGIter);

  mlOp.m_num_mg_iterations = numMGIter;
  int numMGSmooth = 4;
  pp.query("numMGsmooth", numMGSmooth);
  mlOp.m_num_mg_smooth = numMGSmooth;
  mlOp.define(a_grids, a_params.refRatio, vectDomains,
              vectDx, opFactory, lBase);

  BiCGStabSolver<Vector<LevelData<FArrayBox>* > > solver;
  
  bool homogeneousBC = false;
  solver.define(&mlOp, homogeneousBC);
  solver.m_verbosity = a_params.verbosity;
  solver.m_normType = 0;

  solver.solve(a_phi, a_rhs);
   
}
/***************/
void outputData(const Vector<LevelData<FArrayBox>* >&   a_phi,
                const Vector<LevelData<FArrayBox>* >&   a_rhs,
                const Vector< DisjointBoxLayout >&       a_grids, 
                const VCPoissonParameters&                 a_params)
{
#if CH_SPACEDIM==2
    string fileName("vcPoissonOut.2d.hdf5");
#else
    string fileName("vcPoissonOut.3d.hdf5");
#endif

    int nPhiComp = a_phi[0]->nComp();
    int nRhsComp = a_rhs[0]->nComp();
    int totalComp = nPhiComp + nRhsComp;

    Vector<string> phiNames(totalComp);
    // hardwire to single-component
    CH_assert(totalComp == 2);
    phiNames[0] = "phi";
    phiNames[1] = "rhs";


    CH_assert(a_phi.size() == a_rhs.size());
    Vector<LevelData<FArrayBox>* > tempData(a_phi.size(), NULL);
    for (int level=0; level<a_phi.size(); level++)
      {
        tempData[level] = new LevelData<FArrayBox>(a_grids[level], totalComp);
        Interval phiComps(0, nPhiComp-1);
        Interval rhsComps(nPhiComp, totalComp-1);
        a_phi[level]->copyTo(a_phi[level]->interval(),
                             *tempData[level], phiComps);
        a_rhs[level]->copyTo(a_rhs[level]->interval(),
                             *tempData[level], rhsComps);
      }


    Real fakeTime = 1.0;
    Real fakeDt = 1.0;
    WriteAMRHierarchyHDF5(fileName, a_grids,
                          tempData, phiNames, 
                          a_params.coarsestDomain.domainBox(),
                          a_params.coarsestDx,                           
                          fakeDt, fakeTime,
                          a_params.refRatio,
                          a_params.numLevels);
 
    // clean up temporary storage
    for (int level=0; level<a_phi.size(); level++)
      {
        delete tempData[level];
        tempData[level] = NULL;
      }
}




int main(int argc, char* argv[])
{
#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif
  // Scoping trick
  {

    if (argc < 2)
      {
        cerr << " usage " << argv[0] << " <input_file_name> " << endl;
        exit(0);
      }

    char* inFile = argv[1];
    ParmParse pp(argc-2,argv+2,NULL,inFile);

    VCPoissonParameters param;
    Vector<DisjointBoxLayout> grids;

    //read params from file
    getPoissonParameters(param);
    int nlevels = param.numLevels;
    Vector<LevelData<FArrayBox>* > phi(nlevels, NULL);
    Vector<LevelData<FArrayBox>* > rhs(nlevels, NULL);

    setGrids(grids,  param);

    poissonSolve(phi, rhs, grids,  param);

    int dofileout;
    pp.get("write_output", dofileout);
    if(dofileout == 1)
      {
        outputData(phi, rhs, grids, param);
      }
  
    // clear memory
    for (int level = 0; level<phi.size(); level++)
      {
        if (phi[level] != NULL)
          {
            delete phi[level];
            phi[level] = NULL;
          }
        if (rhs[level] != NULL)
          {
            delete rhs[level];
            rhs[level] = NULL;
          }
      }

  }// End scoping trick

#ifdef CH_MPI
  MPI_Finalize();
#endif
}
