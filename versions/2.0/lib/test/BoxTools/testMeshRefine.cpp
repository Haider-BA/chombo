#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

// Purpose:
//  Test the mesh refinement class BRmeshRefine
//
// Usage:
//  <program-name> [-q|-v|-w] ...
//
//  where:
//    -q means run quietly (only pass/fail messages printed)
//    -v means run verbosely (all messages printed)
//    -w means write out the boxes; otherwise will read in
//        boxes from a file and compare with boxes generated
//        in tests.
//    ... all non-option arguments are ignored (Chombo convention)
//
//  Default is `-v'
//
//  Unknown options are treated as errors.
//

// Include files:

#include <cmath>
using std::sqrt;
#include <cstdio>
#include <iostream>
using std::cout;
using std::endl;
#ifdef CH_MPI
#include <mpi.h>
#endif

#include "BRMeshRefine.H"
#include "parstream.H"
#include "LoadBalance.H"
#include "DisjointBoxLayout.H"
#include "CH_HDF5.H"
#include "UsingNamespace.H"

////////////////////////////////////////////////////////////////

/// Prototypes:

void setCircleTags(IntVectSet& ivs, int circleR, int thickness = 1,
                   const IntVect& center= IntVect::Zero) ;

void
parseTestOptions( int argc ,char* argv[] ) ;

void
print_mesh( const Vector<Vector<Box> >& meshVect ,const int maxIndex ,const char* indent ) ;

void
print_mesh( const Vector<Box>& Mesh ,const char* indent ) ;

void
print_ivs( const IntVectSet& ivs ,const char* indent ) ;  //Note: this is in MeshRefine.cpp

std::string
makeOutfileName();

/// Global variables for handling output

static const char *pgmname = "testMeshRefine" ;
static const char *indent = "   " ,*indent2 = "      " ;

static bool verbose = true ;
static bool readFile = true;

/// Code:

int
main(int argc ,char *argv[] )
{

#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif

  const int NLevels = 3 , DefaultRefRatio = 2 ;

  int status = 0,
    BaseLevel = 1,
    TopLevel = NLevels-1,
    newFinestLevel;

  Vector<Box>          domains(NLevels) ;
  Vector<int>          refRatios(NLevels,DefaultRefRatio) ;  //initialize
  Vector<IntVectSet>   tags(NLevels) ,pnds(NLevels) ;

  Real defaultFillRatio = 0.75;
  int defaultBlockFactor = 1;
  int defaultBufferSize = 1;
  int defaultMaxSize = 0;

  BRMeshRefine mrTester;

  parseTestOptions( argc ,argv ) ;
  if( verbose )
    pout() << indent2 << "Beginning " << pgmname << " ..." << endl ;

  ///
  // Test 1 zero lower corner proper nesting test
  ///

  //
  // set up domains to [0,11]
  //
  domains[0] = Box( IntVect::Zero ,11*IntVect::Unit ) ;
  for( int i = 1 ; i < NLevels ; i++ )
    {
      domains[i] = refine( domains[i-1] ,refRatios[i-1] ) ;
    }

  if( verbose )
    {
      pout() << indent2 << "Domains --------" << endl ;
      pout() << indent2 << domains[0] << endl ;
      for( int i = 1 ; i < NLevels ; i++ )
        {
          pout() << indent2 << domains[i] << endl ;
        }
      pout() << indent2 << "----------------" << endl  ;
    }

  // once we've got the domains, can define BRMeshRefine object
  mrTester.define(domains[0], refRatios, defaultFillRatio,
                  defaultBlockFactor, defaultBufferSize,
                  defaultMaxSize);

  //
  // set up meshes
  //
  Vector<Vector<Box> > meshes(NLevels);
  Box tmp[] = { Box( IntVect::Zero ,4*IntVect::Unit ),
                Box( IntVect(D_DECL(5,2,2)) ,7*IntVect::Unit ) } ;
  for (int ibox = 0; ibox < 2; ++ibox)
    {
      meshes[1].push_back(refine(tmp[ibox], refRatios[0]));
    }
  // refine the base level mesh to create the higher levels
  for( int i = 2 ; i < NLevels ; i++ )
    {
      for (int ibox = 0; ibox < meshes[i-1].size(); ++ibox)
        {
          //      Box bref = refine( meshes[i-1][ibox] ,refRatios[i-1] );
          //meshes[i].push_back( bref );
          meshes[i].push_back( refine( meshes[i-1][ibox] ,refRatios[i-1] ) );
        }
    }

  // 0th level mesh is the domain
  meshes[0].push_back(domains[0]);

  if ( verbose )
    {
      pout() << indent2 << "Meshes --------" ;
      print_mesh( meshes ,NLevels-1 ,indent2 ) ;
      pout() << indent2 << "----------------" << endl  ;
    }

  //
  // test PNDs code. do this by passing in tags defined at all
  // grid locations, building grids on levels 2->9, then checking
  // that proper nesting has been enforced.
  //
  {
    Vector<Vector<Box> > newmeshes(NLevels);
    if( verbose )
      {
        pout() << indent2 << "testing makePNDs() with Base=" << BaseLevel << ", Top=" << TopLevel
               << ", BufferSize=1.  Should return ok." << endl ;
      }

    for (int ilev=0; ilev<NLevels; ilev++)
      {
        // tag entire domain for each level
        tags[ilev] = IntVectSet(domains[ilev]);
      }

    int nBuff = 1;
    mrTester.bufferSize(nBuff);
    newFinestLevel = mrTester.regrid(newmeshes,
                                     tags,
                                     BaseLevel,
                                     NLevels-2,
                                     meshes);

    // now test proper nesting
    bool passedPNDTest = true;
    if (newFinestLevel != NLevels-1) passedPNDTest = false;

    for (int ilev=newFinestLevel; ilev > BaseLevel; ilev-- )
      {
        if ( (passedPNDTest) && (ilev > 0))
          {
            IntVectSet crseGridIVS(newmeshes[ilev-1][0]);
            for (int crsebox=0; crsebox<newmeshes[ilev-1].size(); crsebox++)
              {
                crseGridIVS |= newmeshes[ilev-1][crsebox];
              }

            for (int ibox=0; ibox<newmeshes[ilev].size(); ibox++)
              {
                Box thisBox = newmeshes[ilev][ibox];
                thisBox.coarsen(refRatios[ilev-1]);
                thisBox.grow(nBuff);
                thisBox &= domains[ilev];
                if (!crseGridIVS.contains(thisBox))
                  {
                    passedPNDTest = false;
                  }
              }
          }
      } // end loop over levels for PND test

    if( verbose )
      {

        for( int i = 0 ; i < NLevels ; i++ )
          {
            pout() << endl << indent2 << newmeshes[i] ;
          }
        pout() << indent2 << "----------------" << endl  ;
      }
    if( verbose || !passedPNDTest )
      pout() << indent << pgmname << "PND test (buffer=1): "
             << (( passedPNDTest ) ? "passed" : "failed")
             << " lower corner == 0  test." << endl ;
    if (!passedPNDTest) status = 1;
  } // end buffer == 1 test

  // now test with buffer = 2
  //
  {
    Vector<Vector<Box> > newmeshes(NLevels);

    if( verbose )
      {
        pout() << endl << indent2 << "testing makePNDs() with Base=" << BaseLevel << ", Top=" << TopLevel
               << ", BufferSize=2.  Should return ok." << endl ;
      }

    //
    // set up meshes
    //
    Vector<Vector<Box> > meshes(NLevels);
    Box tmp[] = { Box( IntVect::Zero ,4*IntVect::Unit ),
                  Box( IntVect(D_DECL(5,2,2)) ,7*IntVect::Unit ) } ;
    for (int ibox = 0; ibox < 2; ++ibox)
      {
        meshes[1].push_back(refine(tmp[ibox], refRatios[0]));
      }
    // refine the base level mesh to create the higher levels
    for( int i = 2 ; i < NLevels ; i++ )
      {
        for (int ibox = 0; ibox < meshes[i-1].size(); ++ibox)
          {
            //Box bref = refine( meshes[i-1][ibox] ,refRatios[i-1] );
            //meshes[i].push_back( bref );
            meshes[i].push_back(refine(meshes[i-1][ibox],refRatios[i-1]) );
          }
      }

    // 0th level mesh is the domain
    meshes[0].push_back(domains[0]);

    for (int ilev=0; ilev<NLevels; ilev++)
      {
        // tag entire domain for each level
        tags[ilev] = IntVectSet(domains[ilev]);
      }

    int nBuff = 2;
    mrTester.bufferSize(nBuff);
    newFinestLevel = mrTester.regrid(newmeshes,
                                     tags,
                                     BaseLevel,
                                     NLevels-2,
                                     meshes);

    // now test proper nesting
    bool passedPNDTest = true;
    if (newFinestLevel != NLevels-1) passedPNDTest = false;

    for (int ilev=newFinestLevel; ilev > BaseLevel; ilev-- )
      {
        if ( (passedPNDTest) && (ilev > 0))
          {
            IntVectSet crseGridIVS(newmeshes[ilev-1][0]);
            for (int crsebox=0; crsebox<newmeshes[ilev-1].size(); crsebox++)
              {
                crseGridIVS |= newmeshes[ilev-1][crsebox];
              }

            for (int ibox=0; ibox<newmeshes[ilev].size(); ibox++)
              {
                Box thisBox = newmeshes[ilev][ibox];
                thisBox.coarsen(refRatios[ilev-1]);
                thisBox.grow(nBuff);
                thisBox &= domains[ilev-1];
                if (!crseGridIVS.contains(thisBox))
                  {
                    passedPNDTest = false;
                  }
              }
          }
      } // end loop over levels for PND test

    if( verbose )
      {

        for( int i = 0 ; i < NLevels ; i++ )
          {
            pout() << endl << indent2 << newmeshes[i] ;
          }
        pout() << indent2 << "----------------" << endl  ;
      }
    if( verbose || !passedPNDTest )
      pout() << indent << pgmname << "PND test (buffer=2): "
             << (( passedPNDTest ) ? "passed" : "failed")
             << " lower corner == 0  test." << endl ;

    if (!passedPNDTest) status += 2;
  } // end pnd -- buffer = 2 test.

  {
    Vector<Vector<Box> > newmeshes(NLevels);
    bool passedTest = true;

    //
    // test regrid with one tag, scalar IVS
    //
    if( verbose )
      {
        pout() << endl << indent2 << pgmname << ": testing regrid with Base=0,Top=0, scalar IVS w/ 1 tag." << endl ;
        pout() << indent2 << pgmname << ": Should return 1 box on level 1 and one box on level 2" << endl ;
      }

    //
    // set up meshes
    //
    Vector<Vector<Box> > meshes(NLevels);
    // for this test, no pre-existing meshes, so use domains
    for( int ilev = 0 ; ilev < NLevels ; ilev++ )
      {
        meshes[ilev].push_back(domains[ilev]);
      }

    // make a tag set with one tag on BaseLevel.
    tags[0] = IntVectSet( scale( IntVect::Unit,2 ) ) ;
    newFinestLevel = mrTester.regrid( newmeshes ,tags[0] ,0 ,1 ,meshes) ;
    if( verbose )
      {
        pout() << indent2 << "Tags -----------"  ;
        pout() << endl << indent2 << tags[0] ;
        pout() << indent2 << "----------------" << endl  ;
        pout() << indent2 << "New Meshes --------" ;
        print_mesh( newmeshes ,newFinestLevel ,indent2 ) ;
        pout() << indent2 << "-------------------" << endl  ;
      }

    // check results for proper nesting, etc
    // should be 2 levels of refinement
    if (newFinestLevel != 2) passedTest = false;
    const int nBuff = mrTester.bufferSize();
    for (int ilev = newFinestLevel; ilev>0; ilev--)
      {
        // should be one grid per level
        if (newmeshes[ilev].size() != 1) passedTest = false;
        Box newbox(newmeshes[ilev][0]);
        newbox.coarsen(refRatios[ilev-1]);
        newbox.grow(nBuff);
        newbox &= domains[ilev-1];
        // in this case, since blocking factor = 1, the only
        // constraint on grid generation is bufferSize.  therefore,
        // coarse grid should not only contain refined patch +
        // buffer cells, they should be equal
        if (!newmeshes[ilev-1][0].contains(newbox))
          {
            passedTest = false;
          }
        // another quick test -- since level 1 box should just
        // be coarsen(level2box)+buffering
        if (ilev == 2)
          {
            if (newmeshes[ilev-1][0] != newbox)
              {
                passedTest = false;
              }
          }
      }
    if( verbose || !passedTest )
      pout() << indent << pgmname << ": regrid function "
             << (( passedTest ) ? "passed" : "failed")
             << " single tag, scalar IVS test." << endl ;

    if (!passedTest) status += 4;
  }

  {
    bool passedCircleTest = true;
    Vector<Vector<Box> > newmeshes(NLevels);

    //
    // Brian's test with a circle of tags
    //
    int maxsize = 128;
    mrTester.maxSize(maxsize);
    //Box domain(IntVect::Zero, 19*IntVect::Unit);
    IntVect center = 10*IntVect::Unit;
    int thickness = 2 ,circleR = 6 ;
    // set up a tag set
    setCircleTags( tags[1], circleR, thickness, center ) ;
    tags[0].makeEmpty();

    //
    // set up meshes
    //
    Vector<Vector<Box> > meshes(NLevels);
    // for this test, no pre-existing meshes, so use domains
    for( int ilev = 0 ; ilev < NLevels ; ilev++ )
      {
        meshes[ilev].push_back(domains[ilev]);
      }

    if( verbose )
      {
        pout() << endl << indent2 << pgmname << ": testing regrid() with a circle of tags, FillRatio=0.5." << endl ;
        pout() << indent2 << pgmname << ": Should return ok." << endl ;
        pout() << indent2 << "Tags -----------" << endl ;
        pout() << indent2 << tags[1] ;
        pout() << indent2 << "----------------" << endl ;
      }
    mrTester.fillRatio(0.5);
    newFinestLevel = mrTester.regrid(newmeshes, tags, 1, 1, meshes);

#ifdef CH_USE_HDF5
    // compare grids with saved ones (or write them out to the file)
    if (readFile)
      {
        int error;
        HDF5Handle testFile;

        CH_assert(!testFile.isOpen());

        error = testFile.open( makeOutfileName().c_str(), HDF5Handle::OPEN_RDONLY);
        CH_assert( testFile.isOpen() );

        if (error != 0)
          {
            cout << indent2 << "File open failed " << error << endl;
            return error;
          }

        // implicit assumption here that we have the same number
        // of levels as the saved boxes

        for (int ilev=0; ilev<=newFinestLevel; ilev++)
          {
            Vector<Box> savedBoxes;
            char ch[7];
            sprintf(ch, "level_%i", ilev);
            testFile.setGroup(ch);
            error = read(testFile, savedBoxes);
            if (error != 0)
              {
                if (verbose)
                  pout() << indent2 << "box read failed for level "
                       << ilev << " with error code " << error << endl;
                return error;
              }

            // first see if the vectors are the same length
            if (newmeshes[ilev].size() != savedBoxes.size())
              {
                if (verbose)
                  pout() << indent2 << "level " << ilev
                       << " meshes different number of boxes than saved file"
                       << endl;
                passedCircleTest = false;
              }

            // now do box-to-box comparison...
            for (int newBox=0; newBox < newmeshes[ilev].size(); newBox++ )
              {
                const Box& thisNewBox = newmeshes[ilev][newBox];
                bool foundBox = false;
                for (int savedBox = 0; savedBox<savedBoxes.size(); savedBox++)
                  {
                    if (thisNewBox == savedBoxes[savedBox])
                      foundBox = true;
                  }
                if (!foundBox)
                  {
                    pout() << indent2 << "level " << ilev
                           << ": box number " << newBox
                           << " not found in saved box set"
                           << " newBox = " << thisNewBox
                           << endl;
                    passedCircleTest = false;
                  }

              } // end loop over new boxes on this level
          } // end loop over levels

        // must close file or get a
        //      p0_2071:  p4_error: interrupt SIGSEGV: 11
        // from MPI (ndk)
        testFile.close();
      }  // end if we're reading in pre-existing box file
    else
      {
        int error;
        HDF5Handle testFile;

        error =testFile.open( makeOutfileName(), HDF5Handle::CREATE);

        if (error != 0)
          {
            cout << indent2 << "File creation failed " << error << endl;
            return error;
          }

        for (int lev=0; lev<=newFinestLevel; lev++)
          {
            // convert boxes to a BoxLayout
            Vector<int> procAssign(newmeshes[lev].size());

            int loadbalancestatus = LoadBalance(procAssign, newmeshes[lev]);
            CH_assert (loadbalancestatus == 0);

            BoxLayout levelGrids(newmeshes[lev], procAssign);

            CH_assert (testFile.isOpen());
            testFile.setGroupToLevel(lev);
            error = write(testFile, levelGrids);
          }
        if (error != 0)
          {
            cout << indent2 << "box write failed " << error << endl;
          }
        testFile.close();
        CH_assert(!testFile.isOpen());

      }
#endif // CH_USE_HDF5

    if( verbose )
      {
        pout() << indent2 << "New Meshes --------" ;
        print_mesh( newmeshes ,newFinestLevel ,indent2 );
        pout() << indent2 << "-------------------" << endl  ;
      }
    if( verbose || !passedCircleTest )
      pout() << indent << pgmname << ": BRMeshRefine::regrid "
             << (( passedCircleTest ) ? "passed" : "failed")
             << " circle-tags test." << endl ;

    // try again with topLevel = 0, BF=4
    mrTester.blockFactor(4);

    if( verbose )
      {
        pout() << endl << indent2 << pgmname << ": testing regrid() with a circle of tags, FillRatio=0.5, baseLevel = 0, blockFactor =4." << endl ;
        pout() << indent2 << pgmname << ": Should return ok." << endl ;
        pout() << indent2 << "Tags -----------" << endl ;
        pout() << indent2 << tags[1] ;
        pout() << indent2 << "----------------" << endl ;
      }

    newFinestLevel = mrTester.regrid(newmeshes, tags, 0, 1, meshes);

    if( verbose )
      {
        pout() << indent2 << "New Meshes --------" ;
        print_mesh( newmeshes ,newFinestLevel ,indent2 );
        pout() << indent2 << "-------------------" << endl  ;
      }
    // check for proper nesting and for blocking factor
    if (newFinestLevel != 2) passedCircleTest = false;
    const int nBuff = mrTester.bufferSize();
    const int nBlock = mrTester.blockFactor();
    for (int ilev = newFinestLevel; ilev>0; ilev--)
      {
        IntVectSet crseGridIVS(newmeshes[ilev-1][0]);
        for (int crsebox=0; crsebox<newmeshes[ilev-1].size(); crsebox++)
          {
            crseGridIVS |= newmeshes[ilev-1][crsebox];
          }

        // loop over boxes on this level
        for (int igrid=0; igrid<newmeshes[ilev].size(); igrid++)
          {
            Box thisBox(newmeshes[ilev][igrid]);
            // first check blocking factor
            thisBox.coarsen(nBlock);
            thisBox.refine(nBlock);
            if (thisBox != newmeshes[ilev][igrid])
              {
                passedCircleTest = false;
              }
            // now check nesting
            thisBox.coarsen(refRatios[ilev-1]);
            thisBox.grow(nBuff);
            thisBox &= domains[ilev-1];
            if (!crseGridIVS.contains(thisBox))
              {
                passedCircleTest = false;
              }
          } // end loop over new grids
        // final test for disjointness
        Vector<int> procAssign(newmeshes[ilev].size());
        int loadbalancestatus = LoadBalance(procAssign, newmeshes[ilev]);
        if (loadbalancestatus != 0) passedCircleTest = false;
        // this will bomb an assertion if boxes aren't disjoint
        DisjointBoxLayout thisDBL(newmeshes[ilev], procAssign);
        // just to be sure
        if (!thisDBL.isDisjoint()) passedCircleTest = false;
      } // end loop over levels

    if( verbose || !passedCircleTest )
      pout() << indent << pgmname << ": BRMeshRefine::regrid "
             << (( passedCircleTest ) ? "passed" : "failed")
             << " circle-tags test." << endl ;

    if (!passedCircleTest) status += 8;
  }

  ///
  // Test negative lower corner
  ///
  {
    Vector<Vector<Box> > newmeshes(NLevels);
    Vector<Vector<Box> > meshes(NLevels);

    if( verbose )
      {
        pout() << endl << indent2 << "Testing negative box indices" << endl
             << endl ;
      }

    //
    // set up domains to [-2,9]
    //
    domains[0] = Box( -4*IntVect::Unit ,9*IntVect::Unit ) ;
    for( int i = 1 ; i < NLevels ; i++ )
      {
        domains[i] = refine( domains[i-1] ,refRatios[i-1] ) ;
      }

    BRMeshRefine newMrTester(domains[0], refRatios, defaultFillRatio,
                             defaultBlockFactor, defaultBufferSize,
                             defaultMaxSize);
    //
    // set up baselevel and refined meshes
    //
    Box tmp[] = { Box( -4*IntVect::Unit ,7*IntVect::Unit ),
                  Box( IntVect(D_DECL( 8,2,2) ) ,13*IntVect::Unit ) } ;

    meshes[0].push_back(domains[0]);

    for (int ibox = 0; ibox < 2; ++ibox)
      {
        meshes[1].push_back(tmp[ibox]);
      }
    for( int i = 2 ; i < NLevels ; i++ )
      {
        for (int ibox = 0; ibox < meshes[i-1].size(); ++ibox)
          {
            Box bref = refine( meshes[i-1][ibox] ,refRatios[i-1] );
            meshes[i].push_back( bref );
          }
      }

    if( verbose )
      {
        pout() << indent2 << "Domains --------------" ;
        for( int i = 0 ; i < NLevels ; i++ )
          {
            pout() << endl << indent2 << domains[i] ;
          }
        pout() << endl << indent2 << "----------------------" << endl  ;
        pout() << indent2 << "Meshes ---------------" << endl ;
        print_mesh( meshes ,NLevels-1 ,indent2 );
        pout() << endl << indent2 << "----------------------" << endl  ;
      }

    for (int ilev=0; ilev<NLevels; ilev++)
      {
        // tag entire domain for each level
        tags[ilev] = IntVectSet(domains[ilev]);
      }

    int nBuff = 2;
    newMrTester.bufferSize(nBuff);
    newFinestLevel = newMrTester.regrid(newmeshes,
                                        tags,
                                        BaseLevel,
                                        NLevels-2,
                                        meshes);

    // now test proper nesting
    bool passedPNDTest = true;
    if (newFinestLevel != NLevels-1) passedPNDTest = false;

    for (int ilev=newFinestLevel; ilev > BaseLevel; ilev-- )
      {
        if ( (passedPNDTest) && (ilev > 0))
          {
            IntVectSet crseGridIVS(newmeshes[ilev-1][0]);
            for (int crsebox=0; crsebox<newmeshes[ilev-1].size(); crsebox++)
              {
                crseGridIVS |= newmeshes[ilev-1][crsebox];
              }

            for (int ibox=0; ibox<newmeshes[ilev].size(); ibox++)
              {
                Box thisBox = newmeshes[ilev][ibox];
                thisBox.coarsen(refRatios[ilev-1]);
                thisBox.grow(nBuff);
                thisBox &= domains[ilev-1];
                if (!crseGridIVS.contains(thisBox))
                  {
                    passedPNDTest = false;
                  }
              }
          }
      } // end loop over levels for PND test

    if( verbose )
      {
        pout() << indent2 << "New Meshes --------" ;
        for( int i = 0 ; i < NLevels ; i++ )
          {
            pout() << endl << indent2 << newmeshes[i] ;
          }
        pout() << indent2 << "----------------" << endl  ;
      }
    if( verbose || !passedPNDTest )
      pout() << indent << pgmname << " negative lower corner test (buffer=2): "
             << (( passedPNDTest ) ? "passed" : "failed")
             << " lower corner = -4  test." << endl ;

    if (!passedPNDTest) status += 16;
  } // end pnd -- buffer = 2 test.

  // test for variable nRef (4, 2)
  {
    int thisNLevels = 3;
    domains[0] = Box(IntVect::Zero, 11*IntVect::Unit);
    refRatios[0] = 4;
    refRatios[1] = 2;
    refRatios[2] = 2;

    for (int i=1; i<thisNLevels; i++)
      {
    domains[i] = refine(domains[i-1], refRatios[i-1]);
      }

    int bufferSize = 1;
    // once we've got the domains and refRatios, can define BRMeshRefine
    BRMeshRefine latestMRTester;
    // if nRef varies, blockFactor must be at least equal to
    // largest refinement ratio.
    defaultBlockFactor = 4;
    latestMRTester.define(domains[0], refRatios, defaultFillRatio,
              defaultBlockFactor, bufferSize, defaultMaxSize);

    // set up "old" meshes
    Vector<Vector<Box> > oldmeshes(3);

    // level 0 is the domain
    oldmeshes[0].push_back(domains[0]);

    // level 1 is a single box
    Box level1Box(12*IntVect::Unit,47*IntVect::Unit);
    oldmeshes[1].push_back(level1Box);

    // level 2 hasn't been defined yet

    // now define tags to generate new levels
    Vector<IntVectSet> tags(3);

    // level 0 tags are one box (coarsened level 1 box)
    Box level1TagBox(3*IntVect::Unit, 11*IntVect::Unit);
    tags[0] = IntVectSet(level1TagBox);

    // level 1 tags are 2 boxes (coarsened target boxes for level 2)
    Box level2TagBox1(18*IntVect::Unit, 31*IntVect::Unit);
    Box level2TagBox2(36*IntVect::Unit, 47*IntVect::Unit);
    tags[1] = IntVectSet(level2TagBox1);
    tags[1] |= level2TagBox2;

    // now generate grids for level 2
    Vector<Vector<Box> > newmeshes;
    int baseLevel = 0;
    int topLevel = 1;
    int newFinestLevel = latestMRTester.regrid(newmeshes, tags, baseLevel,
                           topLevel, oldmeshes);

    // now test results
    bool passedVariableRefTest = true;

    if (newFinestLevel != 2) passedVariableRefTest = false
;
    // now check to be sure that all tags have been refined
    for (int lev=1; lev<newmeshes.size(); lev++)
      {
    Vector<Box>& levelMeshes = newmeshes[lev];
    IntVectSet& levelTags = tags[lev-1];

    for (int ibox = 0; ibox<levelMeshes.size(); ibox++)
      {
        // coarsen this box down to coarser level, and
        // subtract from tags
        Box thisBox = levelMeshes[ibox];
        thisBox.coarsen(refRatios[lev-1]);
        levelTags -= thisBox;
      }

    // after we've subtracted all the coarsened boxes, tags should be empty
    if (!levelTags.isEmpty()) passedVariableRefTest = false;
      } // end loop over levels

    if (verbose)
      {
    pout() << indent << pgmname << " variable nRef (4,2) test: "
             << (( passedVariableRefTest ) ? "passed" : "failed")
           << endl ;

      }

    if (!passedVariableRefTest) status += 32;

  }

  // Must do all parallel stuff (including pout's) BEFORE MPI_Finalize  (ndk)
  pout() << indent << pgmname << ": "
         << ( (status == 0) ? "passed all tests" : "failed at least one test,")
         << endl;

#ifdef CH_MPI
  MPI_Finalize();
#endif

  return status ;
}

////////////////////////////////////////////////////////////////

void setCircleTags(IntVectSet& ivs, int circleR, int thickness,
                   const IntVect& center )
{
  ivs.define();
  CH_assert(circleR > 0);
  CH_assert(thickness < circleR && thickness > 0);
  for(int t = 0; t<thickness; ++t){
    for(int x=-circleR-t; x<=circleR+t; ++x)
    {
#if CH_SPACEDIM == 1
      ivs |= center + IntVect(x);
#elif CH_SPACEDIM == 2
        // circle
        // variables not used, avoid warning (ndk)
        //IntVect loVect = center+IntVect(x,Y);
        //IntVect hiVect = center+IntVect(x,-Y);
      int Y = (int)sqrt((Real)abs((circleR+t)*(circleR+t) - x*x));
      // not perfect, but I'm not fussy here
        ivs |= center+IntVect(x,Y);
        ivs |= center+IntVect(x,-Y);
#elif CH_SPACEDIM == 3
        //sphere
        int Y = (int)sqrt((Real)abs((circleR+t)*(circleR+t) - x*x)); 
        // not perfect, but I'm not fussy here
        for(int y=-Y; y<=Y; ++y)
          {
            // must cast to Real inside sqrt()  (ndk)
            int Z = (int)sqrt((Real)abs(circleR*circleR - x*x - y*y));
            ivs |= center+IntVect(x,y,Z);
            ivs |= center+IntVect(x,y,-Z);
          }
#else
#error "setCircleTags() implemented only for 2D and 3D"
#endif
    }
  }
}

///
// Parse the standard test options (-v -q) out of the command line.
// The non-standard option -w means write an output file instead of reading input.
// Stop parsing when a non-option argument is found.
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
          else if (strncmp( argv[i], "-w", 3) == 0)
            {
              readFile = false;
              argv[i] = "";
            }
          else
            {
              break ;
            }
        }
    }
  return ;
}

////////////////////////////////////////////////////////////////

///
// Debug functions
///

void
print_mesh( const Vector<Vector<Box> >& meshVect ,const int maxIndex ,const char* indent )
{
  for( int i = 0 ; i <= maxIndex ; i++ )
    {
      char open = '(' ;
      for (int ibox = 0; ibox < meshVect[i].size(); ++ibox)
        {
          pout() << endl << indent << open << meshVect[i][ibox] ;
          open = ' ' ;
        }
        pout() << ')' << endl;
    }
}

void
print_mesh( const Vector<Box>& Mesh ,const char* indent )
{
  char open = '(' ;
  for (int ibox = 0; ibox < Mesh.size(); ++ibox)
    {
      pout() << endl << indent << open << Mesh[ibox] ;
      open = ' ' ;
    }
  pout() << ')' << endl;
}

std::string makeOutfileName()
{
    char dim[3];
    CH_assert( SpaceDim < 100 );
    sprintf( dim, "%d", SpaceDim );

    std::string result( std::string( "meshRefineTest." ) + std::string( dim ) );
#ifdef CH_USE_FLOAT
    result += std::string( "d.FLOAT.H5" );
#else
    result += std::string( "d.H5" );
#endif
    return result;
}