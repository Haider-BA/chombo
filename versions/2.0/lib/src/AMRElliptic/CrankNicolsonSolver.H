#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif
// DFMartin, Sun, May 5, 2002

#ifndef _CrankNicolsonSolver_H_
#define _CrankNicolsonSolver_H_

#include <iostream>
#include <math.h>
#include "SPACE.H"
#include <stdlib.h>
#include <REAL.H>
#include <Box.H>
#include <DisjointBoxLayout.H>
#include <LevelData.H>
#include <FArrayBox.H>
#include <ProblemDomain.H>
#include "LevelHelmholtzSolver.H"
#include "BaseHeatSolver.H"
#include "NamespaceHeader.H"

/// Implements Crank-Nicolson algorithm to update solution to heat equation
/** The CrankNicolsonSolver class implements the Crank-Nicolson algorithm 
    for solving the heat equation.
*/ 
class CrankNicolsonSolver : public BaseHeatSolver
{

public:
  /// default constructor
  CrankNicolsonSolver();

  /// full constructor
  /** Note that the HelholtzOp passed in must already have the
      diffusion coefficient (nu) set.
  */
  CrankNicolsonSolver(const DisjointBoxLayout& a_levelGrids,
                      const DisjointBoxLayout* a_crseGrids,
                      const ProblemDomain& a_domain,
                      Real a_dxLevel,
                      int a_nRefCrse,
                      const BaseHelmholtzOp* a_helmop,
                      int a_ncomp = 1);
  
  /// destructor
  virtual ~CrankNicolsonSolver();

  /// define statement
  /** Note that the HelholtzOp passed in must already have the
      diffusion coefficient (nu) set.
  */
  void define(const DisjointBoxLayout& a_levelGrids,
              const DisjointBoxLayout* a_crseGrids,
              const ProblemDomain& a_domain,
              Real a_dxLevel,
              int a_nRefCrse,
              const BaseHelmholtzOp* a_helmop,
              int a_ncomp = 1);

  /// do solve
  /** update phi
   */
  void updateSoln(LevelData<FArrayBox>& a_phiNew,
                  LevelData<FArrayBox>& a_phiOld,
                  LevelFluxRegister* a_FineFluxRegPtr,
                  LevelFluxRegister* a_CrseFluxRegPtr,
                  const LevelData<FArrayBox>* a_crsePhiOldPtr,
                  Real crseOldTime,
                  const LevelData<FArrayBox>* a_crsePhiNewPtr,
                  Real crseNewTime,
                  /// note that a_src isn't const because of C/F bcs
                  LevelData<FArrayBox>& a_src,
                  Real oldTime,
                  Real dt);


  /// computes time-centered diffusive term for explicit updating
  /** compute time-centered L(phi) for use in subsequent update
      operations.  In this case, we do solve for phiNew, then 
      subtract source and old phi back out to get L(phi).
  */
  void computeDiffusion(LevelData<FArrayBox>& a_DiffusiveTerm,
                        LevelData<FArrayBox>& a_phiOld,
                        LevelFluxRegister* a_FineFluxRegPtr,
                        LevelFluxRegister* a_crseFluxRegPtr,
                        const LevelData<FArrayBox>* a_crsePhiOldPtr,
                        Real crseOldTime,
                        const LevelData<FArrayBox>* a_crsePhiNewPtr,
                        Real crseNewTime,
                        LevelData<FArrayBox>& a_src,
                        Real oldTime,
                        Real dt);

  virtual BaseHeatSolver* new_heatSolver() const;

  virtual void setSolverTolerance(Real a_solverTol);

  virtual Real solverTolerance() const;
  
protected:

  Real m_dx;
  Real m_solverTol;

  BaseHelmholtzOp* m_helmop;
  LevelHelmholtzSolver m_levelSolver;
  ProblemDomain m_domain;

};

#include "NamespaceFooter.H"
#endif


