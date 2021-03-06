#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

//  ANAG, LBNL

#include "FaceIterator.H"
#include "EBGraph.H"

#include "BaseIFFAB.H"
#include <set>
#include <vector>

#include "NamespaceHeader.H"


/********************************/
const Vector<FaceIndex>&
FaceIterator::getVector() const
{
  return m_faces;
}
/********************************/
FaceIterator::FaceIterator()
{
  m_isDefined = false;
}
/********************************/
FaceIterator::~FaceIterator()
{
}
/********************************/
FaceIterator::FaceIterator(const IntVectSet&           a_ivs,
                           const EBGraph&              a_ebgraph,
                           int                         a_direction,
                           const FaceStop::WhichFaces& a_loc)
{
  *this = a_ebgraph.FaceIteratorCache(a_ivs, a_direction, a_loc);

  //define(a_ivs, a_ebgraph, a_direction, a_location);
}

///specialized construction for most typical case, Irregular cells insisde 'region'
FaceIterator::FaceIterator(const Box&     a_region,
                           const EBGraph& a_ebgraph,
                           int            a_direction,
                           const FaceStop::WhichFaces& a_loc)
{

  *this = a_ebgraph.FaceIteratorCache(a_region, a_direction, a_loc);

//   if(a_loc==FaceStop::SurroundingNoBoundary || a_loc == FaceStop::SurroundingWithBoundary)
//     {
//       *this = a_ebgraph.FaceIteratorCache(a_region, a_direction, a_loc);
//     }
//   else
//     {
//       IntVectSet ivs;
//       ivs = a_ebgraph.getIrregCells(a_region);
//       define(ivs, a_ebgraph, a_direction, a_loc);
//     }
}



/********************************/
void
FaceIterator::define(const IntVectSet&           a_ivs,
                     const EBGraph&              a_ebgraph,
                     int                         a_direction,
                     const FaceStop::WhichFaces& a_location)
{

  doDefine(a_ivs, a_ebgraph, a_direction, a_location);

//   if( !s_doUseCache
//   ||  !a_ivs.isDense() )  // TreeIntVectSet::operator< is expensive.
//   {
//     this->doDefine( a_ivs, a_ebgraph, a_direction, a_location );
//   } else
//   {
//     FaceIteratorCache::ConstIteratorType iter;
//     if( TheCache()->containsKey( a_ivs, a_ebgraph, a_direction, a_location,
//                                  &iter ) )
//     {
//       *this = TheCache()->fetch( iter );
//     } else
//     {
//       this->doDefine( a_ivs, a_ebgraph, a_direction, a_location );
//       TheCache()->insert( a_ivs, a_ebgraph, a_direction, a_location,
//                           *this );
//     }
//   }
}

/********************************/
void
FaceIterator::doDefine(const IntVectSet&           a_ivs,
                       const EBGraph&              a_ebgraph,
                       int                         a_direction,
                       const FaceStop::WhichFaces& a_location)
{
  //can't do this because minbox is broken
  //  CH_assert((a_ebgraph.getRegion().contains(a_ivs.minBox())||
  //          a_ivs.isEmpty()));
  m_isDefined = true;
  m_direction = a_direction;

  std::set<FaceIndex, std::less<FaceIndex> > resultSet;

  bool doLo = ((a_location == FaceStop::SurroundingNoBoundary) ||
               (a_location == FaceStop::SurroundingWithBoundary) ||
               (a_location == FaceStop::LoWithBoundary) ||
               (a_location == FaceStop::LoBoundaryOnly) ||
               (a_location == FaceStop::AllBoundaryOnly) ||
               (a_location == FaceStop::LoNoBoundary));
  bool doHi = ((a_location == FaceStop::SurroundingNoBoundary) ||
               (a_location == FaceStop::SurroundingWithBoundary) ||
               (a_location == FaceStop::HiWithBoundary) ||
               (a_location == FaceStop::AllBoundaryOnly) ||
               (a_location == FaceStop::HiBoundaryOnly) ||
               (a_location == FaceStop::HiNoBoundary));
  bool doBoundary = ((a_location == FaceStop::HiWithBoundary) ||
                     (a_location == FaceStop::SurroundingWithBoundary) ||
                     (a_location == FaceStop::LoWithBoundary) ||
                     (a_location == FaceStop::LoBoundaryOnly) ||
                     (a_location == FaceStop::HiBoundaryOnly) ||
                     (a_location == FaceStop::AllBoundaryOnly));
  bool doBoundaryOnly = ((a_location == FaceStop::AllBoundaryOnly) ||
                         (a_location == FaceStop::LoBoundaryOnly) ||
                         (a_location == FaceStop::HiBoundaryOnly));
  bool doInterior = !doBoundaryOnly;

  //if this fails, invalid location.
  CH_assert(doLo || doHi || doBoundaryOnly);

  Side::LoHiSide sides[2] = { doLo ? Side::Lo : Side::Invalid,
                              doHi ? Side::Hi : Side::Invalid };

  for(IVSIterator ivsit(a_ivs); ivsit.ok(); ++ivsit)
    {
      Vector<VolIndex> vofs = a_ebgraph.getVoFs(ivsit());
      for(int ivof=0; ivof<vofs.size(); ++ivof)
        {
          for(int iside=0; iside<2; ++iside )
            {
              if( sides[iside] == Side::Invalid )
                {
                  continue;
                }
              Vector<FaceIndex> faces(
                a_ebgraph.getFaces( vofs[ivof], a_direction, sides[iside] ) );

              for(int iface=0; iface<faces.size(); ++iface)
                {
                  const FaceIndex& face =  faces[iface];
                  if(   (  face.isBoundary()  && doBoundary)
                     || ((!face.isBoundary()) && doInterior) )
                    {
                      resultSet.insert( face );
                    }
                }
            }
        }
    }

  m_faces.clear();
  m_faces.reserve( resultSet.size() );
  for( std::set<FaceIndex>::const_iterator i = resultSet.begin();
       i != resultSet.end();
       ++i )
    {
      m_faces.push_back( *i );
    }
  reset();
}

/********************************/
void
FaceIterator::reset()
{
  CH_assert(isDefined());
  m_iface = 0;
}

/********************************/
//keys off if icell >= m_cellVols.size()
/********************************/
void
FaceIterator::operator++()
{
  CH_assert(isDefined());
  m_iface++;
}

/********************************/
const FaceIndex&
FaceIterator::operator() () const
{
  CH_assert(isDefined());
  CH_assert(m_iface < m_faces.size());
  return m_faces[m_iface];
}

/********************************/
bool
FaceIterator::ok() const
{
  return (m_iface < m_faces.size());
}

/********************************/
bool
FaceIterator::isDefined() const
{
  return m_isDefined;
}


/********************************/


///////////// FaceIteratorCache things from here on down ///////////////
#define FACEITER_CTOR_ARGS   const IntVectSet&           a_ivs,       \
                             const EBGraph&              a_ebgraph,   \
                             int                         a_direction, \
                             const FaceStop::WhichFaces& a_location

/********************************/
// bool
// FaceIteratorCache::containsKey( FACEITER_CTOR_ARGS,
//                                 ConstIteratorType* iter ) const

// {
//     return m_rep.containsKey( a_ivs, a_ebgraph, a_direction, a_location,
//                               iter );
// }

// /********************************/
// void
// FaceIteratorCache::insert( FACEITER_CTOR_ARGS, const FaceIterator& fiter )
// {
//     m_rep.insert( a_ivs, a_ebgraph, a_direction, a_location, fiter );
// }

// /********************************/
// FaceIterator
// FaceIteratorCache::fetch( ConstIteratorType iter ) const
// {
//     return m_rep.fetch( iter );
// }

// /********************************/
// void
// FaceIteratorCache::clear()
// {
//     return m_rep.clear();
// }

// /********************************/
// FaceIteratorCache* FaceIterator::s_cache = 0;


// /********************************/
// /*static*/ FaceIteratorCache*
// FaceIterator::TheCache()
// {
//     if( !s_cache )
//     {
//         s_cache = new FaceIteratorCache;
//         atexit( FaceIterator::DeleteCache );
//         // memtrack.cpp registers its memory-leak summary function with atexit
//         // too, but that happens before the flow of control gets to here, and
//         // so the FaceIterator cache gets cleaned up before memtrack kicks in,
//         // hence no more bogus indication about a memory leak.
//     }
//     return s_cache;
// }

// /*static*/ void
// FaceIterator::DeleteCache()
// {
//   delete s_cache;
//   s_cache = 0;
// }

// /**********************************/

// /*static*/ void
// FaceIterator::setUseCache( bool b )
// {
//     s_doUseCache = b;
// #ifdef CH_MPI
//     if(b)
//       {
//         MayDay::Warning("FaceIterator caching can break in parallel");
//       }
// #endif
// }

// /*static*/ void
// FaceIterator::clearCache()
// {
//     TheCache()->clear();
// }

#undef FACEITER_CTOR_ARGS
#include "NamespaceFooter.H"
