#ifdef CH_LANG_CC
/*
*      _______              __
*     / ___/ /  ___  __ _  / /  ___
*    / /__/ _ \/ _ \/  V \/ _ \/ _ \
*    \___/_//_/\___/_/_/_/_.__/\___/
*    Please refer to Copyright.txt, in Chombo's root directory.
*/
#endif

#include "SPACE.H"
#include "FortranNameMacro.H"
  // Generated by lib/util/multidim/make_FortranNameMacro_H.sh, which is called
  // in turn by the bootstrap script.

//
// Yes, the include guards are here and not at the top of the file.  That's because
// the multidim stuff needs special handling.
//
#ifndef _FORT_PROTO_H_
#define _FORT_PROTO_H_

#include "REAL.H"
#include "NamespaceHeader.H"

#ifdef DOXYGEN
/// Used to declare a fortran routine in a portable way
/** @ingroup macros

 */
#define FORTRAN_NAME( NAME ,name )

/// Used in ChomboFortran to write dimension-indenpendent loops
/** @ingroup macros
CHF_MULTIDO is used to iterate over a box in a dimension independent
fashion by setting up nested Fortran do loops and CHF_ENDDO is used to
terminate those do loops correctly.  Specifically,
CHF_MULTIDO[box;i;j;k] will generate a do loop for  i
nested inside a do loop for  j and, in 3D, this will be nested inside
a do loop for k.  The i loop will go from first element of the
low corner of box to the first element of the high corner of box.
Similarly, the {\tt j} loop will use the second element and, in 3D, the
 k loop will use the third element.  CHF_ENDDO will end all the do
loops set up by  CHF_MULTIDO.

Here is an example using these macros:

\code
      subroutine LOOP(CHF_FRA1[array],CHF_BOX[box])

      integer CHF_DDECL[i;j;k]
      integer productsum

      productsum = 0
      CHF_MULTIDO[box;i;j;k]
        productsum = productsum + i*j*k
        array(CHF_IX[i;j;k]) = productsum
      CHF_ENDDO

      return
      end
\endcode

see the section of the Chombo Design document about Chombo Fortran for
a more detailed use of macros in Fortran

*/
#define CHF_MULTIDO(box, i, j, k)

/** @ingroup macros */
#define CHF_ENDDO

#else

class chfptr {
public:
  int i ;
  //XXX for debugging
  //XXXchfptr(int a):i(a){std::cout << "constructing chfptr " << &i << std::endl;}
  //XXX~chfptr() {std::cout << "destructing chfptr " << &i << std::endl;}
  chfptr(int a):i(a){}
  operator const int*() const {return &i;}
};

#define CHFPTR(a) (const int*)chfptr(a)

#endif

// ChomboFortran declaration macros for Application code

// primitive data types

/** @ingroup macros */
#define CHF_INT( i ) &i
/** @ingroup macros */
#define CHF_CONST_INT( i ) &i
/** @ingroup macros */
#define CHF_REAL( r ) &r
/** @ingroup macros */
#define CHF_CONST_REAL( r ) &r
/** @ingroup macros */
#define CHF_COMPLEX( c ) &c
/** @ingroup macros */
#define CHF_CONST_COMPLEX( c ) &c
/** @ingroup macros */
#define CHF_I1D(       ia,dimen ) ia,(const int*)chfptr(dimen-1)
/** @ingroup macros */
#define CHF_CONST_I1D( ia,dimen ) ia,(const int*)chfptr(dimen-1)
/** @ingroup macros */
#define CHF_R1D(       ra,dimen ) ra,(const int*)chfptr(dimen-1)
/** @ingroup macros */
#define CHF_CONST_R1D( ra,dimen ) ra,(const int*)chfptr(dimen-1)
/** @ingroup macros */
#define CHF_C1D(       ra,dimen ) ra,(const int*)chfptr(dimen-1)
/** @ingroup macros */
#define CHF_CONST_C1D( ra,dimen ) ra,(const int*)chfptr(dimen-1)

// Chombo data types

/** @ingroup macros */
#define CHF_INTVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_CONST_INTVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_REALVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_CONST_REALVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_COMPLEXVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_CONST_COMPLEXVECT( a ) a.dataPtr( )
/** @ingroup macros */
#define CHF_VI(       v ) &v[0],(const int*)chfptr(v.size()-1)
/** @ingroup macros */
#define CHF_CONST_VI( v ) &v[0],(const int*)chfptr(v.size()-1)
/** @ingroup macros */
#define CHF_VR(       v ) &v[0],(const int*)chfptr(v.size()-1)
/** @ingroup macros */
#define CHF_CONST_VR( v ) &v[0],(const int*)chfptr(v.size()-1)
/** @ingroup macros */
#define CHF_VC(       v ) &v[0],(const int*)chfptr(v.size()-1)
/** @ingroup macros */
#define CHF_CONST_VC( v ) &v[0],(const int*)chfptr(v.size()-1)

/** @ingroup macros */
#define CHF_BOX( b ) D_DECL( &b.loVect()[0] \
                            ,&b.loVect()[1] \
                            ,&b.loVect()[2] ) \
                    ,D_DECL( &b.hiVect()[0] \
                            ,&b.hiVect()[1] \
                            ,&b.hiVect()[2] )

#define CHF_BOX_SHIFT( b , iv) D_DECL( CHFPTR(b.loVect()[0] - iv[0]),  \
                                       CHFPTR(b.loVect()[1] - iv[1]),  \
                                       CHFPTR(b.loVect()[2] - iv[2]))  \
                              ,D_DECL( CHFPTR(b.hiVect()[0] - iv[0]), \
                                       CHFPTR(b.hiVect()[1] - iv[1]),  \
                                       CHFPTR(b.hiVect()[2] - iv[2]))

/** @ingroup macros */
#define CHF_FIA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_FRA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_FRA_SHIFT( a , iv) a.dataPtr( 0 ),               \
                     D_DECL( CHFPTR(a.loVect()[0] - iv[0]),  \
                             CHFPTR(a.loVect()[1] - iv[1]),            \
                             CHFPTR(a.loVect()[2] - iv[2]))            \
                    ,D_DECL( CHFPTR(a.hiVect()[0] - iv[0]),    \
                             CHFPTR(a.hiVect()[1] - iv[1]),             \
                             CHFPTR(a.hiVect()[2] - iv[2])) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_FCA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_CONST_FIA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_CONST_FRA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_CONST_FRA_SHIFT( a , iv) a.dataPtr( 0 ),               \
                     D_DECL( CHFPTR(a.loVect()[0] - iv[0]),  \
                             CHFPTR(a.loVect()[1] - iv[1]),            \
                             CHFPTR(a.loVect()[2] - iv[2]))            \
                    ,D_DECL( CHFPTR(a.hiVect()[0] - iv[0]),    \
                             CHFPTR(a.hiVect()[1] - iv[1]),             \
                             CHFPTR(a.hiVect()[2] - iv[2])) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_CONST_FCA( a ) a.dataPtr( 0 ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] ) \
                     ,a.nCompPtr()

/** @ingroup macros */
#define CHF_FIA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

/** @ingroup macros */
#define CHF_FRA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

/** @ingroup macros */
#define CHF_FCA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

/** @ingroup macros */
#define CHF_CONST_FIA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

/** @ingroup macros */
#define CHF_CONST_FRA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

/** @ingroup macros */
#define CHF_CONST_FCA1( a,n ) a.dataPtr( n ) \
                     ,D_DECL( &a.loVect()[0] \
                             ,&a.loVect()[1] \
                             ,&a.loVect()[2] ) \
                     ,D_DECL( &a.hiVect()[0] \
                             ,&a.hiVect()[1] \
                             ,&a.hiVect()[2] )

// Macros used in automatically-generated Chombo Fortran prototypes

// primitive data types

#define CHFp_INT( i ) int* const i
#define CHFp_CONST_INT( i ) const int* const i
#define CHFp_REAL( r ) Real* const r
#define CHFp_CONST_REAL( r ) const Real* const r
#define CHFp_COMPLEX( c ) Complex* const c
#define CHFp_CONST_COMPLEX( c ) const Complex* const c
//[NOTE: the *1D macros has 2 args in C++, but just 1 in Fortran]
#define CHFp_I1D(       ia,dimen )        int* const ia, const int* const i ## ia ## hi0
#define CHFp_CONST_I1D( ia,dimen ) const  int* const ia, const int* const i ## ia ## hi0
#define CHFp_R1D(       ra,dimen )       Real* const ra, const int* const i ## ra ## hi0
#define CHFp_CONST_R1D( ra,dimen ) const Real* const ra, const int* const i ## ra ## hi0
#define CHFp_C1D(       ra,dimen )       Complex* const ra, const int* const i ## ra ## hi0
#define CHFp_CONST_C1D( ra,dimen ) const Complex* const ra, const int* const i ## ra ## hi0

// Chombo data types

#define CHFp_INTVECT( r ) int* const r
#define CHFp_CONST_INTVECT( r ) const int* const r
#define CHFp_REALVECT( r )  Real* const r
#define CHFp_CONST_REALVECT( r ) const Real* const r
#define CHFp_COMPLEXVECT( r )  Complex* const r
#define CHFp_CONST_COMPLEXVECT( r ) const Complex* const r
#define CHFp_VI(       v )        int * const v, const int* const i ## v ## hi0
#define CHFp_CONST_VI( v ) const  int * const v, const int* const i ## v ## hi0
#define CHFp_VR(       v )       Real * const v, const int* const i ## v ## hi0
#define CHFp_CONST_VR( v ) const Real * const v, const int* const i ## v ## hi0
#define CHFp_VC(       v )       Complex * const v, const int* const i ## v ## hi0
#define CHFp_CONST_VC( v ) const Complex * const v, const int* const i ## v ## hi0

#define CHFp_BOX( b ) D_DECL( const int* const i ## b ## lo0 \
                             ,const int* const i ## b ## lo1 \
                             ,const int* const i ## b ## lo2 ) \
                     ,D_DECL( const int* const i ## b ## hi0 \
                             ,const int* const i ## b ## hi1 \
                             ,const int* const i ## b ## hi2 )

#define CHFp_FIA( a ) int *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_FRA( a ) Real *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_FCA( a ) Complex *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_CONST_FIA( a ) const int *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_CONST_FRA( a ) const Real *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_CONST_FCA( a ) const Complex *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 ) \
                             ,const int* const n ## a ## comp
#define CHFp_FIA1( a ) int *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )
#define CHFp_FRA1( a ) Real *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )
#define CHFp_FCA1( a ) Complex *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )
#define CHFp_CONST_FIA1( a ) const int *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )
#define CHFp_CONST_FRA1( a ) const Real *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )
#define CHFp_CONST_FCA1( a ) const Complex *const a \
                     ,D_DECL( const int* const i ## a ## lo0 \
                             ,const int* const i ## a ## lo1 \
                             ,const int* const i ## a ## lo2 ) \
                     ,D_DECL( const int* const i ## a ## hi0 \
                             ,const int* const i ## a ## hi1 \
                             ,const int* const i ## a ## hi2 )

#define CHFt_INT( i )  i
#define CHFt_CONST_INT( i )  i
#define CHFt_REAL( r )  r
#define CHFt_CONST_REAL( r )  r
#define CHFt_COMPLEX( c )  c
#define CHFt_CONST_COMPLEX( c )  c
//[NOTE: the *1D macros has 2 args in C++, but just 1 in Fortran]
#define CHFt_I1D(       ia,dimen )         ia,  i ## ia ## hi0
#define CHFt_CONST_I1D( ia,dimen )   ia,  i ## ia ## hi0
#define CHFt_R1D(       ra,dimen )        ra,  i ## ra ## hi0
#define CHFt_CONST_R1D( ra,dimen )  ra,  i ## ra ## hi0
#define CHFt_C1D(       ra,dimen )        ra,  i ## ra ## hi0
#define CHFt_CONST_C1D( ra,dimen )  ra,  i ## ra ## hi0

// Chombo data types

#define CHFt_INTVECT( r )  r
#define CHFt_CONST_INTVECT( r )  r
#define CHFt_REALVECT( r )   r
#define CHFt_CONST_REALVECT( r )  r
#define CHFt_COMPLEXVECT( r )   r
#define CHFt_CONST_COMPLEXVECT( r )  r
#define CHFt_VI(       v )       v,  i ## v ## hi0
#define CHFt_CONST_VI( v )  v,  i ## v ## hi0
#define CHFt_VR(       v )       v,  i ## v ## hi0
#define CHFt_CONST_VR( v ) v,  i ## v ## hi0
#define CHFt_VC(       v )      v,  i ## v ## hi0
#define CHFt_CONST_VC( v ) v,  i ## v ## hi0

#define CHFt_BOX( b ) D_DECL(  i ## b ## lo0 \
                             , i ## b ## lo1 \
                             , i ## b ## lo2 ) \
                     ,D_DECL(  i ## b ## hi0 \
                             , i ## b ## hi1 \
                             , i ## b ## hi2 )

#define CHFt_FIA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_FRA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_FCA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_CONST_FIA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_CONST_FRA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_CONST_FCA( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 ) \
                             , n ## a ## comp
#define CHFt_FIA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )
#define CHFt_FRA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )
#define CHFt_FCA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )
#define CHFt_CONST_FIA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )
#define CHFt_CONST_FRA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )
#define CHFt_CONST_FCA1( a ) a \
                     ,D_DECL(  i ## a ## lo0 \
                             , i ## a ## lo1 \
                             , i ## a ## lo2 ) \
                     ,D_DECL(  i ## a ## hi0 \
                             , i ## a ## hi1 \
                             , i ## a ## hi2 )

#include "NamespaceFooter.H"
#endif
