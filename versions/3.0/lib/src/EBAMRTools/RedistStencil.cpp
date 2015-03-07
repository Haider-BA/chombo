#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

// dtgraves weds oct 3 2001

#include "REAL.H"
#include "FArrayBox.H"
#include "PolyGeom.H"
#include "LevelData.H"
#include "DisjointBoxLayout.H"
#include "EBISLayout.H"
#include "EBCellFAB.H"
#include "Interval.H"
#include "RedistStencil.H"
#include "VoFIterator.H"
#include "EBIndexSpace.H"
#include "EBArith.H"
#include "NamespaceHeader.H"
/**************************/
/**************************/
int
RedistStencil::
getRedistRadius() const
{
  return m_redistRadius;
}
/**************************/
/**************************/
RedistStencil::
RedistStencil()
{
  m_isDefined = false;
}
/**************************/
/**************************/
RedistStencil::
~RedistStencil()
{
}
/**************************/
/**************************/
RedistStencil::
RedistStencil(const DisjointBoxLayout& a_dbl,
                    const EBISLayout& a_ebisl,
                    const ProblemDomain& a_domain,
                    const int& a_redistRadius)
{
  define(a_dbl, a_ebisl, a_domain, a_redistRadius);
}
/**************************/
/**************************/
void
RedistStencil::
define(const DisjointBoxLayout& a_dbl,
       const EBISLayout& a_ebisl,
       const ProblemDomain& a_domain,
       const int& a_redistRadius)
{
  m_isDefined = true;
  m_hasDefaultWeights = true;
  m_grids = a_dbl;
  m_ebisl = a_ebisl;
  m_domain = a_domain;
  m_redistRadius = a_redistRadius;
  m_stencil.define(m_grids);
  m_volsten.define(m_grids);
  for(DataIterator dit = m_grids.dataIterator(); dit.ok(); ++dit)
    {
      Box region = m_grids.get(dit());
      //at some point i convinced myself that this was necessary.  right now
      //i cannot think of a case where it it is.
      //      region.grow(2*m_redistRadius);
      region.grow(m_redistRadius);
      region &= a_domain;
      const EBISBox& ebisBox = m_ebisl[dit()];
      IntVectSet irregIVS = ebisBox.getIrregIVS(region);
      BaseIVFAB<VoFStencil >& stenFAB =    m_stencil[dit()];
      BaseIVFAB<VoFStencil >& volstenFAB = m_volsten[dit()];
      stenFAB.define(   irregIVS, ebisBox.getEBGraph(), 1);
      volstenFAB.define(irregIVS, ebisBox.getEBGraph(), 1);
      for(VoFIterator vofit(irregIVS, ebisBox.getEBGraph()); vofit.ok(); ++vofit)
        {
          const VolIndex& vof = vofit();
          VoFStencil thisSten;
          computePointStencil(thisSten, vof,  dit());
          stenFAB(vof, 0) = thisSten;
          volstenFAB(vof, 0) = thisSten;
        }
    }
}
/**************************/
/**************************/
bool
RedistStencil::
isDefined() const
{
  return m_isDefined;
}
/**************************/
/**************************/
void
RedistStencil::
resetWeights(const LevelData<EBCellFAB>& a_modifier,
             const int& a_ivar)
{
  CH_assert(isDefined());
  m_hasDefaultWeights = false;
  for(DataIterator dit = m_grids.dataIterator(); dit.ok(); ++dit)
    {
      const EBISBox& ebisBox = m_ebisl[dit()];
      //initiate with the volume weighted stencil
      BaseIVFAB<VoFStencil >& stenFAB = m_stencil[dit()];
      BaseIVFAB<VoFStencil >& volstenFAB = m_volsten[dit()];
      const EBCellFAB& modFAB = a_modifier[dit()];
      const IntVectSet& irregIVS = volstenFAB.getIVS();
      for(VoFIterator vofit(irregIVS, ebisBox.getEBGraph()); vofit.ok(); ++vofit)
        {
          const VolIndex& vof = vofit();
          VoFStencil oldSten = volstenFAB(vof, 0);
          VoFStencil newSten;
          Real sum = 0.0;
          for(int isten = 0; isten < oldSten.size(); isten++)
            {
              const VolIndex& thatVoF = oldSten.vof(isten);

              Real weight  = modFAB(thatVoF, a_ivar);
              Real volfrac = ebisBox.volFrac(thatVoF);
              //it is weight*volfrac that is normalized
              sum += weight*volfrac;
              newSten.add(thatVoF, weight);
            }
          if(Abs(sum) > 0.0)
            {
              Real scaling = 1.0/sum;
              newSten *= scaling;
            }
          stenFAB(vof, 0) = newSten;
        }
    }
}
/**************************/
/**************************/
const BaseIVFAB<VoFStencil >&
RedistStencil::
operator[](const DataIndex& a_datInd) const
{
  return m_stencil[a_datInd];
}
/**************************/
/**************************/
void
RedistStencil::
deepCopy(const RedistStencil& a_stenin)
{
  define(a_stenin.m_grids, a_stenin.m_ebisl,
         a_stenin.m_domain, a_stenin.m_redistRadius);
  //in the case where its weights are other than the defaults,
  //need to copy the stencil explicitly.

  if(!a_stenin.m_hasDefaultWeights)
    {
      Interval interv(0,0);
      for(DataIterator dit = m_grids.dataIterator(); dit.ok(); ++dit)
        {
          Box copyBox = grow(m_grids.get(dit()), 2*m_redistRadius);
          m_stencil[dit()].copy(copyBox, interv,  copyBox,
                                a_stenin.m_stencil[dit()], interv);
        }
    }
}
/**************************/
/**************************/
void
RedistStencil::
computePointStencil(VoFStencil& a_stencil,
                    const VolIndex&   a_srcVoF,
                    const DataIndex&  a_datInd)
{
  //get the vofs.  these are the intvects
  //it must be called with the first time
  const EBISBox& ebisBox = m_ebisl[a_datInd];
  IntVect timesMoved = IntVect::Zero;
  IntVect pathSign   = IntVect::Zero;
  Vector<VolIndex> vofsStencil;
  EBArith::getAllVoFsInMonotonePath(vofsStencil, timesMoved,
                                    pathSign, a_srcVoF, ebisBox,
                                    m_redistRadius);

  //now set the weights according to the volumefrac/sum(volfrac)
  //you can reset the weights later if you like
  a_stencil.clear();
  Real sum = 0.0;
  for(int isten = 0; isten < vofsStencil.size(); isten++)
    {
      const VolIndex& vof = vofsStencil[isten];
      Real weight = ebisBox.volFrac(vof);
      //w*volfrac is normalized
      //so the default weight is 1/sum
      sum += weight;
      //add with the sum of
      a_stencil.add(vof, 1.0);
    }
  //normalize the stencil
  if(Abs(sum) > PolyGeom::getTolerance())
    {
      Real scale = 1.0/sum;
      a_stencil *= scale;
    }
  //if there is not enough volume to which to redistribute 
  //set the stencil to zero.   "Enough volume" shall be defined
  //as at least the volume of one full cell
  if(Abs(sum) < 1.0)
    {
      a_stencil *= 0.0;
    }
}
#include "NamespaceFooter.H"
