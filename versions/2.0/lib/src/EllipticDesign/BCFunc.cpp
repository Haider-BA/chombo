#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

#include "BCFunc.H"
#include "RealVect.H"
#include "BoxIterator.H"
#include "NamespaceHeader.H"
//#include "BCFuncF_F.H"

inline
Real quadraticInterp(const Real& a_inhomogVal,
                     const Real& a_nearVal, 
                     const Real& a_farVal)
{
  Real retval  = (8.0/3.0)*a_inhomogVal + (1.0/3.0)*(a_farVal) -2*(a_nearVal);
  return retval;
}

inline
Real linearInterp(const Real& a_inhomogVal, 
                  const Real& a_nearVal)
{

  // ignore value at x=1
  Real retval = 2*a_inhomogVal-a_nearVal;
  return retval;
}

void NeumBC(FArrayBox&  a_state,
            const Box&  a_valid,
            Real        a_dx,
            bool        a_homogeneous,
            BCValueFunc a_value)
{
  for(int idir = 0; idir < SpaceDim; idir++)
    {
      for(SideIterator sit; sit.ok(); ++sit)
        {
          NeumBC(a_state, a_valid, a_dx, a_homogeneous, a_value, idir,sit());
        }
    }
}
void
getDomainFacePosition(RealVect&             a_retval,
                      const IntVect&        a_validIV,
                      const Real&           a_dx,
                      const int&            a_dir,
                      const Side::LoHiSide& a_side)
{
  Real* dataPtr = a_retval.dataPtr();
  
  D_TERM( dataPtr[0] = a_dx*(a_validIV[0] + 0.5);,\
          dataPtr[1] = a_dx*(a_validIV[1] + 0.5);,\
          dataPtr[2] = a_dx*(a_validIV[2] + 0.5);)

  int isign = sign(a_side);
  dataPtr[a_dir] += 0.5*Real(isign)*a_dx;
}
void NeumBC(FArrayBox&     a_state,
            const Box&     a_valid,
            Real           a_dx,
            bool           a_homogeneous,
            BCValueFunc    a_value,
            int            a_dir,
            Side::LoHiSide a_side)
{
  
  int isign = sign(a_side);
  RealVect facePos;
  Box to_region = adjCellBox(a_valid, a_dir, a_side, 1);
  Box fromRegion = to_region;
  fromRegion.shift(a_dir, -isign);
  a_state.copy(a_state, fromRegion, 0, to_region, 0, a_state.nComp());
  if(!a_homogeneous)
    {
      Real* value  = new Real[a_state.nComp()];
      for(BoxIterator bit(to_region); bit.ok(); ++bit)
        {
          const IntVect& ivTo = bit();
          IntVect ivClose = ivTo -   isign*BASISV(a_dir);
          if(!a_homogeneous)
            {
              getDomainFacePosition(facePos, ivClose, a_dx, a_dir, a_side);

              
              a_value(facePos.dataPtr(), &a_dir, &a_side, value);
              for(int icomp = 0; icomp < a_state.nComp(); icomp++)
                {
                  a_state(ivTo, icomp) += Real(isign)*a_dx*value[icomp];
                }
            }
        }
      delete[] value;
    }

}

void  DiriBC(FArrayBox&  a_state,
             const Box&  a_valid,
             Real        a_dx,
             bool        a_homogeneous,
             BCValueFunc a_value,
             int         a_order)
{
  for(int idir = 0; idir < SpaceDim; idir++)
    {
      for(SideIterator sit; sit.ok(); ++sit)
        {
          
          DiriBC(a_state, a_valid, a_dx, a_homogeneous, a_value, idir, sit(), a_order);
        }
    }
}
void  DiriBC(FArrayBox&     a_state,
             const Box&     a_valid,
             Real           a_dx,
             bool           a_homogeneous,
             BCValueFunc    a_value,
             int            a_dir,
             Side::LoHiSide a_side,
             int            a_order)
{
  int isign = sign(a_side);
  RealVect facePos;
  Box to_region = adjCellBox(a_valid, a_dir, a_side, 1);
  Real* value  = new Real[a_state.nComp()];
  for(BoxIterator bit(to_region); bit.ok(); ++bit)
    {
      const IntVect& ivTo = bit();
      IntVect ivClose = ivTo -   isign*BASISV(a_dir);
      IntVect ivFar   = ivTo - 2*isign*BASISV(a_dir);
      if(!a_homogeneous)
        {
          getDomainFacePosition(facePos, ivClose, a_dx, a_dir, a_side);
          a_value(facePos.dataPtr(), &a_dir, &a_side, value);
        }
      for(int icomp = 0; icomp < a_state.nComp(); icomp++)
        {
          Real nearVal = a_state(ivClose, icomp);
          Real  farVal = a_state(ivFar,   icomp);
          Real inhomogVal = 0;
          if(!a_homogeneous)
            {
              inhomogVal = value[icomp];
            }
          Real ghostVal;
          if(a_order == 1)
            {
              ghostVal = linearInterp(inhomogVal, nearVal);
            }
          else if(a_order == 2)
            {
              ghostVal = quadraticInterp(inhomogVal, nearVal, farVal);
            }
          else
            {
              MayDay::Error("bogus order argument");
            }

          a_state(ivTo, icomp) = ghostVal;
        }
    }
  delete[] value;
}

void  ReflectiveVectorBC(FArrayBox&     a_state,
                         const Box&     a_valid,
                         Real           a_dx,
                         int            a_dir,
                         Side::LoHiSide a_side,
                         int            a_order)
{
  int isign = sign(a_side);
  RealVect facePos;
  Box to_region = adjCellBox(a_valid, a_dir, a_side, 1);
  for(BoxIterator bit(to_region); bit.ok(); ++bit)
    {
      const IntVect& ivTo = bit();
      IntVect ivClose = ivTo -   isign*BASISV(a_dir);
      IntVect ivFar   = ivTo - 2*isign*BASISV(a_dir);
      Real inhomogVal = 0;
      for(int icomp = 0; icomp < a_state.nComp(); icomp++)
        {
          Real ghostVal;
          Real nearVal = a_state(ivClose, icomp);
          //zero diri for normal dir
          if(icomp == a_dir)
            {
              Real  farVal = a_state(ivFar,   icomp);
              if(a_order == 1)
                {
                  ghostVal = linearInterp(inhomogVal, nearVal);
                }
              else if(a_order == 2)
                {
                  ghostVal = quadraticInterp(inhomogVal, nearVal, farVal);
                }
            }
          else
            {
              //zero neumann for tang dir
              ghostVal = nearVal;
            }
          a_state(ivTo, icomp) = ghostVal;
        }
    }
}


void  NoSlipVectorBC(FArrayBox&     a_state,
                     const Box&     a_valid,
                     Real           a_dx,
                     int            a_dir,
                     Side::LoHiSide a_side,
                     int            a_order)
{
  int isign = sign(a_side);
  RealVect facePos;
  Box to_region = adjCellBox(a_valid, a_dir, a_side, 1);
  for(BoxIterator bit(to_region); bit.ok(); ++bit)
    {
      const IntVect& ivTo = bit();
      IntVect ivClose = ivTo -   isign*BASISV(a_dir);
      IntVect ivFar   = ivTo - 2*isign*BASISV(a_dir);
      Real inhomogVal = 0;
      for(int icomp = 0; icomp < a_state.nComp(); icomp++)
        {
          Real nearVal = a_state(ivClose, icomp);
          Real  farVal = a_state(ivFar,   icomp);
          Real ghostVal;
          if(a_order == 1)
            {
              ghostVal = linearInterp(inhomogVal, nearVal);
            }
          else if(a_order == 2)
            {
              ghostVal = quadraticInterp(inhomogVal, nearVal, farVal);
            }

          a_state(ivTo, icomp) = ghostVal;
        }
    }
}

#include "NamespaceFooter.H"
