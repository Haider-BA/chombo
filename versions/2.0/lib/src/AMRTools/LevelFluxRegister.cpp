#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

#include "LevelFluxRegister.H"
#include "LevelFluxRegisterF_F.H"
#include "CH_Timer.H"
#include "LayoutIterator.H"
//#include "DebugOut.H"
#include "Copier.H"
#include "parstream.H"
#include "BoxIterator.H"
#include "NamespaceHeader.H"

void
LevelFluxRegister::poutCoarseRegisters() const
{
  for(DataIterator dit = m_coarFlux.dataIterator(); dit.ok(); ++dit)
    {
      pout() << "dumping coarse registers" << endl;
      const FArrayBox& fab = m_coarFlux[dit()];
      const Box&       box = fab.box();
      for(BoxIterator bit(box); bit.ok(); ++bit)
        {
          if(Abs(fab(bit(), 0)) > 0.0)
            {
              pout() << bit()  << "  ";
              for(int ivar = 0; ivar < fab.nComp(); ivar++)
                {
                  pout() << fab(bit(), ivar) << "  ";
                }
              pout() << endl;
            }
        }
    }
}

void
LevelFluxRegister::poutFineRegisters() const
{
  for(DataIterator dit = m_fineFlux.dataIterator(); dit.ok(); ++dit)
    {
      pout() << "dumping fine registers" << endl;
      const FArrayBox& fab = m_fineFlux[dit()];
      const Box&       box = fab.box();
      for(BoxIterator bit(box); bit.ok(); ++bit)
        {
          if(Abs(fab(bit(), 0)) > 0.0)
            {
              pout() << bit()  << "  ";
              for(int ivar = 0; ivar < fab.nComp(); ivar++)
                {
                  pout() << fab(bit(), ivar) << "  ";
                }
              pout() << endl;
            }
        }
    }
}
LevelFluxRegister::LevelFluxRegister(const DisjointBoxLayout& a_dblFine,
                                     const DisjointBoxLayout& a_dblCoar,
                                     const ProblemDomain&     a_dProblem,
                                     int                      a_nRefine,
                                     int                      a_nComp,
                                     bool                     a_scaleFineFluxes)
{
  define(a_dblFine, a_dblCoar, a_dProblem,
         a_nRefine, a_nComp, a_scaleFineFluxes);
}

LevelFluxRegister::LevelFluxRegister(const DisjointBoxLayout& a_dblFine,
                                     const DisjointBoxLayout& a_dblCoar,
                                     const Box&               a_dProblem,
                                     int                      a_nRefine,
                                     int                      a_nComp,
                                     bool                     a_scaleFineFluxes)
{

  ProblemDomain physDomain(a_dProblem);
  define(a_dblFine, a_dblCoar, physDomain,
         a_nRefine, a_nComp, a_scaleFineFluxes);
}

void
LevelFluxRegister::define(const DisjointBoxLayout& a_dbl,
                          const DisjointBoxLayout& a_dblCoarse,
                          const ProblemDomain&     a_dProblem,
                          int                      a_nRefine,
                          int                      a_nComp,
                          bool                     a_scaleFineFluxes)
{
  m_isDefined = true;
  m_nRefine   = a_nRefine;
  m_coarFlux.define( a_dblCoarse, a_nComp);
  m_domain = a_dProblem;
  m_scaleFineFluxes = a_scaleFineFluxes;

  DisjointBoxLayout coarsenedFine;
  coarsen(coarsenedFine, a_dbl, a_nRefine);

  ProblemDomain coarsenedDomain(a_dProblem);
  coarsenedDomain.coarsen(a_nRefine);

  m_fineFlux.define( coarsenedFine, a_nComp, IntVect::Unit);

  m_reverseCopier.ghostDefine(coarsenedFine, a_dblCoarse,
                              coarsenedDomain, IntVect::Unit);

  for(int i=0; i<CH_SPACEDIM; i++)
    {
      m_coarseLocations[i].define(a_dblCoarse);
      m_coarseLocations[i+CH_SPACEDIM].define(a_dblCoarse);
    }
 

  DataIterator dC   = a_dblCoarse.dataIterator();
  LayoutIterator dF = coarsenedFine.layoutIterator();

  for(dC.begin(); dC.ok(); ++dC)
    {
      const Box& cBox = a_dblCoarse.get(dC);

      for(dF.begin(); dF.ok(); ++dF)
        {
          const Box& fBox  = coarsenedFine.get(dF);

          if(fBox.bigEnd(0)+1 < cBox.smallEnd(0))
            {
              //can skip this box since they cannot intersect, due to sorting
            }
          else if (fBox.smallEnd(0)-1 > cBox.bigEnd(0))
            {
              //skip to end, since all the rest of boxes will not intersect either
              dF.end();
            }
          else
            {

              for(int i=0; i<CH_SPACEDIM; i++)
                {
                  Vector<Box>& lo = m_coarseLocations[i][dC];
                  Vector<Box>& hi = m_coarseLocations[i+CH_SPACEDIM][dC];

                  Box loBox = adjCellLo(fBox, i, 1);
                  Box hiBox = adjCellHi(fBox, i, 1);
                  if(cBox.intersectsNotEmpty(loBox)) lo.push_back(loBox & cBox);
                  if(cBox.intersectsNotEmpty(hiBox)) hi.push_back(hiBox & cBox);
                }
            }
        }
    }

  Box domainBox = coarsenedDomain.domainBox();

  if(a_dProblem.isPeriodic())
    {
      Vector<Box> periodicBoxes[2*CH_SPACEDIM];
      for(dF.begin(); dF.ok(); ++dF)
        {
          const Box& fBox  = coarsenedFine.get(dF);
          for(int i=0; i<CH_SPACEDIM; i++)
            {
              if(a_dProblem.isPeriodic(i))
                {
                  if(fBox.smallEnd(i) == domainBox.smallEnd(i))
                    periodicBoxes[i].push_back(adjCellLo(fBox, i, 1));
                  if(fBox.bigEnd(i) == domainBox.bigEnd(i))
                    periodicBoxes[i+CH_SPACEDIM].push_back(adjCellHi(fBox, i, 1));
                }
            }
        }
      for(int i=0; i<CH_SPACEDIM; i++)
        {
          Vector<Box>& loV = periodicBoxes[i];
          Vector<Box>& hiV = periodicBoxes[i+CH_SPACEDIM];
          int size = domainBox.size(i);
          for(int j=0; j<loV.size(); j++) loV[j].shift(i, size);
          for(int j=0; j<hiV.size(); j++) hiV[j].shift(i, -size);
        }
      for(dC.begin(); dC.ok(); ++dC)
        {
          const Box& cBox = a_dblCoarse.get(dC);
          for(int i=0; i<CH_SPACEDIM; i++)
            if(a_dProblem.isPeriodic(i))
              {
                Vector<Box>& loV = periodicBoxes[i];
                Vector<Box>& hiV = periodicBoxes[i+CH_SPACEDIM];

                if(cBox.smallEnd(i) == domainBox.smallEnd(i) )
                  {
                    Vector<Box>& hi = m_coarseLocations[i+CH_SPACEDIM][dC];
                    for(int j=0; j<hiV.size(); j++)
                      {
                        if(cBox.intersectsNotEmpty(hiV[j])) hi.push_back(cBox & hiV[j]);
                      }
                  }
                if(cBox.bigEnd(i) == domainBox.bigEnd(i) )
                  {
                    Vector<Box>& lo = m_coarseLocations[i][dC];
                    for(int j=0; j<loV.size(); j++)
                      {
                        if(cBox.intersectsNotEmpty(loV[j])) lo.push_back(cBox & loV[j]);
                      }
                  }

              }
        }
    }

}

void
LevelFluxRegister::define(const DisjointBoxLayout& a_dbl,
                          const DisjointBoxLayout& a_dblCoarse,
                          const Box&               a_dProblem,
                          int                      a_nRefine,
                          int                      a_nComp,
                          bool                     a_scaleFineFluxes)
{
  ProblemDomain physDomain(a_dProblem);
  define(a_dbl, a_dblCoarse, physDomain,
         a_nRefine, a_nComp, a_scaleFineFluxes);
}

LevelFluxRegister::LevelFluxRegister()
{
  m_scaleFineFluxes = true;
  m_isDefined = false;
}

LevelFluxRegister::~LevelFluxRegister()
{
}

bool
LevelFluxRegister::isDefined() const
{
  return m_isDefined;
}

void
LevelFluxRegister::undefine()
{
  m_isDefined = false;
}

void
LevelFluxRegister::setToZero()
{
  DataIterator d =  m_coarFlux.dataIterator();
  for(d.begin(); d.ok(); ++d)
    {
      m_coarFlux[d].setVal(0.0);
    }
  d =   m_fineFlux.dataIterator();
  for(d.begin(); d.ok(); ++d)
    {
      m_fineFlux[d].setVal(0.0);
    }

}

void
LevelFluxRegister::incrementCoarse(FArrayBox& a_coarseFlux,
                                   Real a_scale,
                                   const DataIndex& a_coarseDataIndex,
                                   const Interval& a_srcInterval,
                                   const Interval& a_dstInterval,
                                   int a_dir)

{
 CH_assert(isDefined());
  incrementCoarse(a_coarseFlux, a_scale, a_coarseDataIndex, a_srcInterval,
                  a_dstInterval, a_dir, Side::Lo);
  incrementCoarse(a_coarseFlux, a_scale, a_coarseDataIndex, a_srcInterval,
                  a_dstInterval, a_dir, Side::Hi);
}

void
LevelFluxRegister::incrementCoarse(FArrayBox& a_coarseFlux,
                                   Real a_scale,
                                   const DataIndex& a_coarseDataIndex,
                                   const Interval& a_srcInterval,
                                   const Interval& a_dstInterval,
                                   int a_dir,
                                   Side::LoHiSide a_sd)
{
 CH_assert(isDefined());
  const Vector<Box>& intersect =
    m_coarseLocations[a_dir+a_sd*CH_SPACEDIM][a_coarseDataIndex];

  FArrayBox& coarse = m_coarFlux[a_coarseDataIndex];

  a_coarseFlux.shiftHalf(a_dir, sign(a_sd));
  Real scale = -sign(a_sd)*a_scale;
  int s = a_srcInterval.begin();
  int d = a_dstInterval.begin();
  int size = a_srcInterval.size();

  for(int b=0; b<intersect.size(); ++b)
    {
      const Box& box = intersect[b];
      coarse.plus(a_coarseFlux, box, box, scale, s, d, size);
    }

  a_coarseFlux.shiftHalf(a_dir, - sign(a_sd));
}

void
LevelFluxRegister::incrementFine(FArrayBox& a_fineFlux,
                                 Real a_scale,
                                 const DataIndex& a_fineDataIndex,
                                 const Interval& a_srcInterval,
                                 const Interval& a_dstInterval,
                                 int a_dir)
{
  CH_assert(isDefined());
  incrementFine(a_fineFlux, a_scale, a_fineDataIndex, a_srcInterval,
                a_dstInterval, a_dir, Side::Lo);
  incrementFine(a_fineFlux, a_scale, a_fineDataIndex, a_srcInterval,
                a_dstInterval, a_dir, Side::Hi);
}

void
LevelFluxRegister::incrementFine(FArrayBox& a_fineFlux,
                                 Real a_scale,
                                 const DataIndex& a_fineDataIndex,
                                 const Interval& a_srcInterval,
                                 const Interval& a_dstInterval,
                                 int a_dir,
                                 Side::LoHiSide a_sd)
{
  CH_TIME("LevelFluxRegister::incrementFine");
  CH_assert(isDefined());
 
  a_fineFlux.shiftHalf(a_dir, sign(a_sd));
  Real denom = 1.0;

  if (m_scaleFineFluxes)
  {
    denom = D_TERM(1, *m_nRefine, *m_nRefine);
  }

  Real scale = sign(a_sd)*a_scale/denom;

  FArrayBox& cFine = m_fineFlux[a_fineDataIndex];
  //  FArrayBox  cFineFortran(cFine.box(), cFine.nComp());
  //  cFineFortran.copy(cFine);

  Box clipBox = m_fineFlux.box(a_fineDataIndex);
  clipBox.refine(m_nRefine);
  Box fineBox;
  if(a_sd == Side::Lo)
    {
      fineBox = adjCellLo(clipBox, a_dir, 1);
      fineBox &= a_fineFlux.box();
    }
  else
    {
      fineBox = adjCellHi(clipBox,a_dir,1);
      fineBox &= a_fineFlux.box();
    }
#if 0
  for(BoxIterator b(fineBox); b.ok(); ++b)
     {
       int s = a_srcInterval.begin();
       int d = a_dstInterval.begin();
       for(;s<=a_srcInterval.end(); ++s, ++d)
         {
           cFine(coarsen(b(), m_nRefine), d) += scale*a_fineFlux(b(), s);
         }
     }
#else
   // shifting to ensure fineBox is in the positive quadrant, so IntVect coarening
   // is just integer division.

 

   const IntVect& iv=fineBox.smallEnd();
   IntVect civ = coarsen(iv, m_nRefine);
   FORT_INCREMENTFINE(CHF_CONST_FRA_SHIFT(a_fineFlux, iv),
                      CHF_FRA_SHIFT(cFine, civ),
                      CHF_BOX_SHIFT(fineBox, iv),
                      CHF_CONST_INT(m_nRefine),
                      CHF_CONST_REAL(scale));




   //fineBox.shift(-shift);
   // now, check that cFineFortran and cFine are the same
   //fineBox.coarsen(m_nRefine);
//    for(BoxIterator b(fineBox); b.ok(); ++b)
//      {
//        if(cFineFortran(b(),0) != cFine(b(),0))
//          {
//            MayDay::Error("Fortran doesn't match C++");
//          }
//      }
   // need to shift boxes back to where they were on entry.

   //  cFine.shift(-shift/m_nRefine);
#endif
  a_fineFlux.shiftHalf(a_dir, - sign(a_sd));

}

void
LevelFluxRegister::reflux(LevelData<FArrayBox>& a_uCoarse,
                          Real a_scale)
{
  Interval interval(0, a_uCoarse.nComp()-1);
  reflux(a_uCoarse, interval, interval, a_scale);
}

class AddOp : public LDOperator<FArrayBox>
{
public:
  Real scale;
  AddOp():scale(1.0)
  {
  }

  virtual void linearIn(FArrayBox& arg,  void* buf, const Box& R,
                        const Interval& comps) const
  {
    Real* buffer = (Real*)buf;
    if(scale != 1.0)
    {
      ForAllXBNNnoindx(Real, arg, R, comps.begin(), comps.size())
      {
        argR+=*buffer * scale;
        buffer++;
      } EndFor
    }
    else
    {
      ForAllXBNNnoindx(Real, arg, R, comps.begin(), comps.size())
      {
        argR+=*buffer;
        buffer++;
      } EndFor
    }
  }

  void op(FArrayBox& dest,
          const Box& RegionFrom,
          const Interval& Cdest,
          const Box& RegionTo,
          const FArrayBox& src,
          const Interval& Csrc) const
  {
    if(scale != 1.0)
      dest.plus(src, RegionFrom, RegionTo, scale, Csrc.begin(), Cdest.begin(), Cdest.size());
    else
      dest.plus(src, RegionFrom, RegionTo, Csrc.begin(), Cdest.begin(), Cdest.size());
  }
};

void LevelFluxRegister::reflux(
                               LevelData<FArrayBox>& a_uCoarse,
                               const Interval&       a_coarse_interval,
                               const Interval&       a_flux_interval,
                               Real                  a_scale )
{
  CH_TIME("LevelFluxRegister::reflux");
  for(DataIterator dit(a_uCoarse.dataIterator()); dit.ok(); ++dit)
    {
      FArrayBox& u = a_uCoarse[dit];
      FArrayBox& coarse = m_coarFlux[dit];
      u.plus(coarse, -a_scale, a_flux_interval.begin(), 
             a_coarse_interval.begin(), a_coarse_interval.size() );
    }
  AddOp op;
  op.scale = -a_scale;
  m_fineFlux.copyTo(a_flux_interval, a_uCoarse, a_coarse_interval, m_reverseCopier, op);

}

void LevelFluxRegister::reflux(
                               LevelData<FArrayBox>& a_uCoarse,
                               Real                  a_scale,
                               const Interval&       a_coarse_interval,
                               const Interval&       a_flux_interval,
                               const LevelData<FArrayBox>& a_beta
                               )
{
  CH_TIME("LevelFluxRegister::reflux");
  CH_assert(a_beta.nComp() ==1);
  LevelData<FArrayBox> increment(a_uCoarse.disjointBoxLayout(), a_uCoarse.nComp(), a_uCoarse.ghostVect());
  for(DataIterator dit(a_uCoarse.dataIterator()); dit.ok(); ++dit)
    {
      increment[dit()].setVal(0.0);
    }

  reflux(increment, a_coarse_interval, a_flux_interval, a_scale );
  for(DataIterator dit(a_uCoarse.dataIterator()); dit.ok(); ++dit)
    {
      for(int icomp = 0; icomp < increment.nComp(); icomp++)
        {
          int srccomp = 0;
          int dstcomp = icomp;
          int ncomp = 1;
          increment[dit()].mult(a_beta[dit()], srccomp, dstcomp, ncomp);
        }
      int srccomp = 0;
      int dstcomp = 0;
      int ncomp = a_uCoarse.nComp();
      a_uCoarse[dit()].plus(increment[dit()], srccomp, dstcomp, ncomp);
    }
}
#include "NamespaceFooter.H"
