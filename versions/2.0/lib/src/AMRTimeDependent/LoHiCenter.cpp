#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

#include "LoHiCenter.H"
#include "NamespaceHeader.H"

// This function is used when in direction a_dir a 3 point stencil of cell-
// centered data is being used to compute something at the cell center of the
// center cell of the stencil.  The data for the stencil is valid in
// a_inBox.  It divides a_inBox into a cell-centered box where the full
// stencil can be used, a_centerBox, and cell-centered boxes where 2 point
// stencils must be used, a_loBox and a_hiBox based on the current problem
// domain, a_domain, and the stencil direction, a_dir.  The union of these 2
// and 3 point stencil boxes is returned as a_entireBox.  The 2 point stencil
// boxes are one wide, at most, and if they have been defined then the
// corresponding flag, a_hasLo or a_hasHi, is set to one, otherwise these
// flags are zero.  All output boxes lie within the domain.

void loHiCenter(Box&                 a_loBox,
                int&                 a_hasLo,
                Box&                 a_hiBox,
                int&                 a_hasHi,
                Box&                 a_centerBox,
                Box&                 a_entireBox,
                const Box&           a_inBox,
                const ProblemDomain& a_domain,
                const int&           a_dir)
{
  // Make a copy of the input box which can be modified
  Box inBox = a_inBox;
  inBox &= a_domain;

  // The centered difference box is always one smaller in a_dir
  a_centerBox = inBox;
  a_centerBox.grow(a_dir,-1);

  // The union of all the output boxes start off equal to the center
  // difference portion on the input box (intersected with the domain)
  a_entireBox = a_centerBox;

  // See if this chops off the high side of the input box
  Box tmp = a_inBox;
  tmp.shift(a_dir,-1);
  tmp &= a_domain;
  tmp.shift(a_dir,1);

  // If so, set up the high, one-sided difference box, a_hiBox, and expand
  // the entire box to include it
  if (!a_domain.contains(tmp))
  {
    a_hasHi = 1;
    tmp.shift(a_dir,-2);
    a_hiBox = adjCellHi(tmp,a_dir);
    a_entireBox.growHi(a_dir,1);
  }
  else
  {
    a_hasHi = 0;
    a_hiBox = Box();
  }

  // See if this chops off the low side of the input box
  tmp = a_inBox;
  tmp.shift(a_dir,1);
  tmp &= a_domain;
  tmp.shift(a_dir,-1);

  // If so, set up the low, one-sided difference box, a_loBox, and expand
  // the entire box to include it
  if (!a_domain.contains(tmp))
  {
    a_hasLo = 1;
    tmp.shift(a_dir,2);
    a_loBox = adjCellLo(tmp,a_dir);
    a_entireBox.growLo(a_dir,1);
  }
  else
  {
    a_hasLo = 0;
    a_loBox = Box();
  }

  // Make some simple sanity checks
  CH_assert(a_entireBox.contains(a_centerBox));

  if (a_hasLo == 1)
  {
    CH_assert(a_entireBox.contains(a_loBox));
  }

  if (a_hasHi == 1)
  {
    CH_assert(a_entireBox.contains(a_hiBox));
  }
}

// This function is used when in direction a_dir a 2 point stencil of cell-
// centered data is being used to compute something at the cell face between
// the cell centers of the stencil.  The data for the stencil is valid in
// a_inBox.  It uses a_inBox to compute a box (face-centered in a_dir) where
// the full stencil can be used, a_centerBox, and boxes (face-centered in
// a_dir) where a 1 point stencil must be used, a_loBox and a_hiBox based on
// the current problem domain, a_domain, and the stencil direction, a_dir.
// The union of these 1 and 2 point stencil boxes is returned as a_entireBox
// (face-centered in a_dir).  The 1 point stencil boxes are one wide, at most,
// and if they have been defined then the corresponding flag, a_hasLo or
// a_hasHi, is set to one, otherwise these flags are zero.  All output boxes
// lie within the domain.

void loHiCenterFace(Box&                 a_loBox,
                    int&                 a_hasLo,
                    Box&                 a_hiBox,
                    int&                 a_hasHi,
                    Box&                 a_centerBox,
                    Box&                 a_entireBox,
                    const Box&           a_inBox,
                    const ProblemDomain& a_domain,
                    const int&           a_dir)
{
  // Make a copy of the input box which can be modified
  Box inBox = a_inBox;
  inBox &= a_domain;

  // The centered difference box is always one smaller in a_dir
  a_centerBox = inBox;
  a_centerBox.surroundingNodes(a_dir);
  a_centerBox.grow(a_dir,-1);

  // The union of all the output boxes start off equal to the center
  // difference portion on the input box (intersected with the domain)
  a_entireBox = a_centerBox;

  // See if this chops off the high side of the input box
  Box tmp = a_inBox;
  tmp.shift(a_dir,-1);
  tmp &= a_domain;
  tmp.shift(a_dir,1);

  // If so, set up the high, one-sided difference box, a_hiBox, and expand
  // the entire box to include it
  if (!a_domain.contains(tmp))
  {
    a_hasHi = 1;
    tmp.shift(a_dir,-2);
    a_hiBox = adjCellHi(tmp,a_dir);
    a_hiBox.shiftHalf(a_dir,1);
    a_entireBox.growHi(a_dir,1);
  }
  else
  {
    a_hasHi = 0;
    a_hiBox = Box().convert(BASISV(a_dir));
  }

  // See if this chops off the low side of the input box
  tmp = a_inBox;
  tmp.shift(a_dir,1);
  tmp &= a_domain;
  tmp.shift(a_dir,-1);

  // If so, set up the low, one-sided difference box, a_loBox, and expand
  // the entire box to include it
  if (!a_domain.contains(tmp))
  {
    a_hasLo = 1;
    tmp.shift(a_dir,2);
    a_loBox = adjCellLo(tmp,a_dir);
    a_loBox.shiftHalf(a_dir,-1);
    a_entireBox.growLo(a_dir,1);
  }
  else
  {
    a_hasLo = 0;
    a_loBox = Box().convert(BASISV(a_dir));
  }

  // Make some simple sanity checks
  CH_assert(a_entireBox.contains(a_centerBox));

  if (a_hasLo == 1)
  {
    CH_assert(a_entireBox.contains(a_loBox));
  }

  if (a_hasHi == 1)
  {
    CH_assert(a_entireBox.contains(a_hiBox));
  }
}
#include "NamespaceFooter.H"
