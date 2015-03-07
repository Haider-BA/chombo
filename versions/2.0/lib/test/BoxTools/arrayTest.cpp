#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

// test for mutual exclusion gaurds and include order things

#include <cmath>
#include <cstdlib>
#include <iostream>

#include "LevelData.H"
#include "BoxLayoutData.H"
#include "LevelData.H"
#include "DataIterator.H"
#include "Copier.H"
#include "SPMD.H"
#include "BRMeshRefine.H"
#include "IntVectSet.H"
#include "LoadBalance.H"
#include "LayoutIterator.H"
#include "Misc.H"
#include "BoxIterator.H"
#include "CH_Timer.H"
#include "UsingNamespace.H"

using std::cout;
using std::endl;

/// Global variables for test output:

static const char *pgmname = "arrayTest" ;
static const char *indent = "   " ,*indent2 = "      " ;
static bool verbose = true ;

/// Prototypes:

void
setCircleTags(IntVectSet& ivs, int circleR, int thickness = 1,
              const IntVect& center= IntVect::Zero);
void
buildDisjointBoxLayout(DisjointBoxLayout& plan,
                       const IntVectSet& tags, const Box& domain);
void
parseTestOptions( int argc ,char* argv[] ) ;

/// Code:

int
main(int argc, char** argv)
{
#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif
  bool success = true ;
  {

  parseTestOptions( argc ,argv ) ;
  if( verbose )
    cout << indent2 << "Beginning " << pgmname << " ..." << endl ;

  DisjointBoxLayout plan1, plan2;
  {
    Box domain(IntVect::Zero, 20*IntVect::Unit);

    IntVectSet tags;

    IntVect center = 10*IntVect::Unit;

    setCircleTags(tags, 6, 1, center); //circle/sphere

    buildDisjointBoxLayout(plan1, tags, domain);

    tags.makeEmpty();

    setCircleTags(tags, 5, 2, center);

    buildDisjointBoxLayout(plan2, tags, domain);
  }

  {
    BoxLayout shallowBL(plan1); // shallow copy operation

    if(!(shallowBL == plan1)){
      cout << indent << pgmname
           << ": BoxLayout FAILED shallow copy construction test: dest != src" << endl ;
      success = false ;
    }

    BoxLayout deepBL;
    deepBL.deepCopy(plan1);

    if(deepBL == plan1){
      cout << indent << pgmname
           << ": BoxLayout::deepCopy() FAILED: dest not different from src" << endl;
      success = false ;
    }

    if(!deepBL.compatible(plan1)){
      cout << indent <<pgmname
           <<":  BoxLayout::deepCopy() FAILED: dest not compatible with src" <<endl;
      success = false;
    // both shallowBL and deepBL go out of scope here, ensure that this
    // doesn't have side-effects.
    }
  }

  LevelData<BaseFab<int> > level1(plan1, 3, IntVect::Unit);

  LevelData<BaseFab<int> > level2;

  level2.define(level1);

  level2.define(plan2, 1);

  for(DataIterator i(level2.dataIterator()); i.ok(); ++i)
  {
    level2[i()].setVal(2);
  }

  struct values
  {
    static void setVal1(const Box& box, int comps, BaseFab<int>& fab)
      {
        int center = (box.smallEnd()[0] + box.bigEnd()[0])/2;
        fab.setVal(center, 0);
        fab.setVal(2*center, 1);
        fab.setVal(3*center, 2);

      }
    static void setVal2(const Box& box, int comps, BaseFab<int>& fab)
      {
        // this is to make this test also work in 1d
        int dir = min(1, SpaceDim-1);
        int center = (box.smallEnd()[dir] + box.bigEnd()[dir])/2;
        fab.setVal(center);
      }

  };

  level1.apply(values::setVal1);

  level2.apply(values::setVal2);

  LayoutData<IntVectSet> masks(level1.boxLayout());

  for(DataIterator it(level1.dataIterator()); it.ok(); ++it)
    {
      Box rightSide = level1.box(it());
      // this is to make this test also work in 1d
      int dir = min(1, SpaceDim-1);
      rightSide = adjCellHi(rightSide, dir);
      Box leftSide = level1.box(it());
      leftSide = adjCellLo(leftSide, dir);
      masks[it()]|= rightSide;
      masks[it()]|= leftSide;
    }

  CH_TIMER_REPORT();
  CH_TIMER_RESET();

  DisjointBoxLayout refinedPlan;
  refine(refinedPlan, plan1, 2);
 CH_assert(refinedPlan.isClosed());

  DisjointBoxLayout recoarsenedPlan;
  coarsen(recoarsenedPlan, refinedPlan, 2);
 CH_assert(recoarsenedPlan.isClosed());

  for(LayoutIterator it(plan1.layoutIterator()); it.ok(); ++it)
    {
      if(plan1.get(it()) != recoarsenedPlan.get(it()))
        {
          cout << indent << pgmname
               << " FAILED: refine() then coarsen() does not seem to be reversible." << endl ;
          success = false ;
          if(verbose)
            cout << indent2 << plan1[it()] << endl;
        }
    }

  // copyTo testing.  Invocation testing with many boxes.....
  level1.copyTo(level2.interval(), level2, level2.interval());

  level1.exchange(level1.interval());

  level1.exchange(Interval(0,0));

  level2.copyTo(level2.interval(), level1, Interval(1,1));

  // copyTo testing.  Hardcore testing with just a couple boxes.
  // b1 and b2 are disjoint, b3 overlaps both of them.
  Box b1(IntVect(D_DECL(1,2,1)), IntVect(D_DECL(4,4,4)));
  Box b2(IntVect(D_DECL(5,2,1)), IntVect(D_DECL(12,4,4)));
  Box b3(IntVect(D_DECL(3,2,2)), IntVect(D_DECL(6,15,4)));
  Box b4(IntVect(D_DECL(-2,3,-1)), IntVect(D_DECL(0,5,0)));
  Box b5(IntVect(D_DECL(-4,-2,1)), IntVect(D_DECL(-2,0,6)));

  Box  dm(IntVect(D_DECL(-5, -5, -5)), IntVect(D_DECL(5,5,5)));

  ProblemDomain domain(dm);

  DisjointBoxLayout bl1, bl2;
  DataIndex d1, d2, d3, d4, d5;
  bl1.addBox(b2, procID()); 
  bl1.addBox(b1, procID());
  bl2.addBox(b3, procID()); 
  bl1.addBox(b4, procID());
  bl2.addBox(b5, procID());
  bl1.close(); bl2.close();

  for(DataIterator dit=bl1.dataIterator(); dit.ok(); ++dit)
    {
      if(bl1[dit] == b1) d1=dit();
      if(bl1[dit] == b2) d2=dit();
      if(bl1[dit] == b4) d4=dit();
    }
  for(DataIterator dit=bl2.dataIterator(); dit.ok(); ++dit)
    {
      if(bl2[dit] == b3) d3=dit();
      if(bl2[dit] == b5) d5=dit();
    }

  BoxLayout bbl1, bbl2;
  bbl1.deepCopy(bl1);   bbl1.close();
  bbl2.deepCopy(bl2);   bbl2.close();

  {
    // check that LayoutIterator returns boxes in sorted order correctly
    LayoutIterator lit1(bl1.layoutIterator());
    LayoutIterator lit2(bl2.layoutIterator());
    if(d4 == lit1() && d5 == lit2())
    {
      ++lit1; ++lit2 ;
      if(d1 == lit1() && d3 == lit2())
      {
        ++lit1; ++ lit2;
        if(d2 == lit1())
        {
          ++lit1; ++lit2; //over-increment lit2
          if(!lit1.ok() && !lit2.ok())
          {
            //passed the end of the iterators, make sure now NOT ok()
            // good, everything is in order
            goto passOrder;
          }
        }
      }
    }
    cout << indent << pgmname
         << ": LayoutIterator FAILED to return objects in order after close" << endl ;
    success = false ;

  passOrder:
    ;
  } // lit1 and lit2 pass out of scope here, make sure there are no side effects

  LevelData<BaseFab<int> > l1(bl1, 1, 2*IntVect::Unit);
  LevelData<BaseFab<int> > l2(bl2, 2);
  BoxLayoutData<BaseFab<int> > regl1(bbl1, 1);
  BoxLayoutData<BaseFab<int> > regl2(bbl2, 2);

  LayoutData<Vector<RefCountedPtr<BaseFab<int> > > > destData;

  l1[d1].setVal(1);  l1[d2].setVal(2); l1[d4].setVal(800);
  l2[d3].setVal(30, 0); l2[d3].setVal(33, 1); //leave d5 uninitialized

  regl1[d1].setVal(1);  regl1[d2].setVal(2); regl1[d4].setVal(800);
  regl2[d3].setVal(30, 0); regl2[d3].setVal(33, 1); //leave d5 uninitialized

  regl1.generalCopyTo(bbl2, destData, regl1.interval(), domain);

  if(destData[d3].size() != 2)
    {
      cout<<"missed a box in generalCopyTo\n";
      success = false;
    }

  l1.copyTo(l1.interval(), l2, l1.interval());

  BaseFab<int>& T1(l1[d1]); // handy way to clean up the code below
  BaseFab<int>& T2(l1[d2]);
  BaseFab<int>& T3(l2[d3]);
  BaseFab<int>& regT3(regl2[d3]);

  regl2[d3].copy(*(destData[d3][0]),0,0);
  regl2[d3].copy(*(destData[d3][1]),0,0);
  for(BoxIterator bit(b3); bit.ok(); ++bit)
    {
      if(regT3(bit()) != T3(bit()))
        {
          cout<< indent << pgmname
              << "generalCopyTo not equivalent to copyTo operation"<<endl;
          success = false;
        }
    }
  for(BoxIterator bit(b1); bit.ok(); ++bit)
    {
      if(T1(bit()) != 1)
        {
          cout << indent << pgmname
               << ": copyTo test #1 FAILED.  Source data was modified." << endl ;
          success = false ;
        }
    }
  for(BoxIterator bit(b2); bit.ok(); ++bit)
    {
      if(T2(bit()) != 2)
        {
          cout << indent << pgmname
               << ": copyTo test #2 FAILED.  Source data was modified." << endl ;
          success = false ;
        }
    }
  for(BoxIterator bit( b3 & b2);  bit.ok(); ++bit)
    {
      if(T3(bit(), 0) != 2 || T3(bit(), 1) != 33)
        {
          cout << indent << pgmname
               << ": copyTo test #3 FAILED. Dest data not modified correctly." << endl ;
          success = false ;
        }
    }
  for(BoxIterator bit( b3 & b1);  bit.ok(); ++bit)
    {
      if(T3(bit(), 0) != 1 || T3(bit(), 1) != 33)
        {
          cout << indent << pgmname
               << ": copyTo test #4 FAILED. Dest data not modified correctly." << endl ;
          success = false ;
        }
    }

  for(BoxIterator bit(b3); bit.ok(); ++bit)
    {
      if(b1.contains(bit()) || b2.contains(bit()))
       ; //skip
      else
        {
          if(T3(bit(), 0) != 30 || T3(bit(), 1) != 33)
            {
              cout << indent << pgmname
                   << ": copyTo test #5 FAILED.  coypyTo() corrupted data that shouldn't be changed." << endl ;
              success = false ;
            }
        }
    }

  }
  if(success)
    cout << indent << pgmname << ": ArrayTest seems to have passed" << endl;
#ifdef CH_MPI
  MPI_Finalize();
#endif

  return 0;
}

////////////////////////////////////////////////////////////////

void
setCircleTags(IntVectSet& ivs, int circleR, int thickness,
              const IntVect& center)
{
 CH_assert(circleR > 0);
 CH_assert(thickness < circleR && thickness > 0);
  for(int t = 0; t<thickness; ++t){
    for(int x=-circleR-t; x<=circleR+t; ++x)
    {
      // not perfect, but I'm not fussy here

#if CH_SPACEDIM == 1
      ivs |= center + IntVect(x);
#elif CH_SPACEDIM == 2
      // circle
      int Y = (int)sqrt((Real)abs(circleR*circleR - x*x));
      ivs |= center+IntVect(x,Y);
      ivs |= center+IntVect(x,-Y);
#elif CH_SPACEDIM == 3
      //sphere
      int Y = (int)sqrt((Real)abs(circleR*circleR - x*x));
      for(int y=-Y; y<=Y; ++y)
      {
        Real rrxxyy = Abs(circleR*circleR - x*x - y*y);
        int Z = (int)sqrt(rrxxyy);
        ivs |= center+IntVect(x,y,Z);
        ivs |= center+IntVect(x,y,-Z);
      }
#else
#error arrayTest failed: implemented only for 2D and 3D
#endif
    }
  }
}

void
buildDisjointBoxLayout(DisjointBoxLayout& plan,
                       const IntVectSet& tags, const Box& domain)
{

  Vector<Vector<Box> > vectBox(1);
  Vector<Vector<int> > assignments(1);
  Vector<Vector<long> > computeLoads(1);

  IntVectSet domainivs(domain);
  int maxsize = 128;
  Vector<int> fakeNRef(2,2);
  Real fillRatio = 0.5;
  int bufferSize = 0;
  int blockFactor = 1;
  BRMeshRefine mr(domain, fakeNRef, fillRatio, blockFactor,
                  bufferSize, maxsize);
  mr.makeBoxes(vectBox[0], tags, domainivs, domain, maxsize, 0);

  assignments[0].resize(vectBox[0].size());
  computeLoads[0].resize(vectBox[0].size());

  for(int i=0; i<vectBox[0].size(); ++i)
    {
      computeLoads[0][i] = vectBox[0][i].numPts();
    }
  Real eff; Vector<int> refRatio(1,1);
  int stat = LoadBalance(assignments, eff, vectBox, computeLoads, refRatio);
  if( stat != 0 ) {
    MayDay::Error("loadBalance() FAILED.",stat);
  }

  plan.define(vectBox[0], assignments[0]);

  plan.close();
}

///
// Parse the standard test options (-v -q) out of the command line.
///
void
parseTestOptions( int argc ,char* argv[] )
{
  for( int i = 1 ; i < argc ; ++i )
    {
      if( argv[i][0] == '-' ) //if it is an option
        {
          // compare 3 chars to differentiate -x from -xx
          if( strncmp( argv[i] ,"-v" ,3 ) == 0 )
            {
              verbose = true ;
              argv[i] = "" ;
            }
          else if( strncmp( argv[i] ,"-q" ,3 ) == 0 )
            {
              verbose = false ;
              argv[i] = "" ;
            }
        }
    }
  return ;
}
