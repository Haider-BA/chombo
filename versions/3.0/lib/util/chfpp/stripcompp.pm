#!/usr/local/gnu/bin/perl -w
####  _______              __
#### / ___/ /  ___  __ _  / /  ___
####/ /__/ _ \/ _ \/  ' \/ _ \/ _ \
####\___/_//_/\___/_/_/_/_.__/\___/ 
####
####
#### This software is copyright (C) by the Lawrence Berkeley
#### National Laboratory.  Permission is granted to reproduce
#### this software for non-commercial purposes provided that
#### this notice is left intact.
#### 
#### It is acknowledged that the U.S. Government has rights to
#### this software under Contract DE-AC03-765F00098 between
#### the U.S.  Department of Energy and the University of
#### California.
####
#### This software is provided as a professional and academic
#### contribution for joint exchange. Thus it is experimental,
#### is provided ``as is'', with no warranties of any kind
#### whatsoever, no support, no promise of updates, or printed
#### documentation. By using this software, you acknowledge
#### that the Lawrence Berkeley National Laboratory and
#### Regents of the University of California shall have no
#### liability with respect to the infringement of other
#### copyrights by any part of this software.
####
#################################################
###  This is the comment remover
### Interface is
### sub StripComProc::StripComments(inputfile, outputfile,
###                                 SpaceDim, debug)
###
###  reads in input file,
###  removes all whole-line Fortran comments and trailing "!" comments
###  reformats statement labels to be right-justified because
###   GNU cpp version 3 messes them up
###   (this is ugly but not worth a separate routine <dbs>)
###  writes the result
###
#################################################

package StripComProc;
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);
use Exporter;
$VERSION = 2.00;
@ISA = qw(Exporter);
@EXPORT = qw(&stripComments);
@EXPORT_OK = qw(&stripComments);
@EXPORT_TAGS= ();

sub StripComProc::StripComments
{

    use strict;
    my ($FINFile, $FOUTFile, $SpaceDim, $debug) = @_;
    if($debug)
    {
        print "StripComProc: \n";
        print "input file  = $FINFile \n";
        print "output file = $FOUTFile \n";
        print "SpaceDim    = $SpaceDim \n";
    }
    
    open(FOUT,">" . $FOUTFile) 
        or die "Error: cannot open output file " . $FOUTFile . "\n";
    open(FIN,"<" . $FINFile) 
        or die "Error: cannot open input file " . $FINFile . "\n";

    my @tmplines = <FIN>;  ## slurp mode
    my (@outbuf, $ibuf);
    my $openquote = 0;  ## =1 if a string was started on a previous line
    while (@tmplines) 
    {
        $ibuf = shift(@tmplines);
###     skip lines that start with c, ! or *
        if($ibuf =~ m/^c/i)
        {
            next;
        }
        elsif($ibuf =~ m/^\s*\!/)
        {
            next;
        }
        elsif($ibuf =~ m/^\*/)
        {
            next;
        }
###     skip blank lines too
        elsif($ibuf =~ m/^\s*$/)
        {
            next;
        }
        else
        {
            # strip trailing comments that start with "!"
            # but watch out for "!" in strings
            ( $ibuf,$openquote ) = &StripComProc::stripTrailingComment( $ibuf,$openquote,$debug );
            if($ibuf =~ m/^[ \d]{5}/)
            {   # right-justify statement label in first 5 columns;
            # this wont detect invalid code and probably wont handle 
                # tab-formatted code correctly, but that isn't standard anyway.
                #[NOTE: this works around a bug in 'g++ -E' in version 3.x <dbs>]
                my @label = split //,$ibuf,6;  #6th element is rest of string
                my $j = 4 ;
                for( my $i=4 ; $i>=0 ; --$i ){
                    # move digits to the right
                    if( $label[$i] =~ /\d/ ){
                        if( $i < $j ){
                            $label[$j] = $label[$i] ; $label[$i] = " " ;
                        }
                        --$j ;
                    }
                }
                push @outbuf,join("",@label);
            }
            else
            {
                push @outbuf,$ibuf;
            }
        }
    }
    print FOUT @outbuf;
    

    ###need to close files to make this modular.
    close(FIN);
    close(FOUT);
    return 1;   
}


#### Remove trailing comment, if any, and return the new line. 
#### Comments start with "!" except when it is in a string
sub StripComProc::stripTrailingComment
{
    use strict;
    my ($ibuf ,$openquote ,$debug ) = @_;
    # remove and save any line terminators
    $ibuf =~ s/[\n\r]*$// ;
    my $lineterm = $& ;
    if($debug and $openquote)
    {
        print "StripComProc::stripTrailingComment: starting with an open string \n";
        print "line = [$ibuf]\n";
    }
    # find a comment char that isn't in a string, depending on
    # whether there is already an open string on a previous line
    #[NOTE: if this line ends with an open string, no comment is allowed. <dbs>]
    if( $openquote )
    {
        # already in a string, so look first for the close quote,
        # then strings and ordinary chars, then finally the comment
        if( $ibuf =~ m/^(([^\'\"]*[\'\"])?([^\'\"\!]*([\'\"][^\'\"]*[\'\"])*)*)(\s*\!.*)?$/ )
        {
            if( $debug ){ print "stripTrailingComment: open |$1|$5|\n"; }
            $ibuf = $1;
        }
    }
    else
    {
        # the pattern is:
        # ([not_a_quote_or_comment]*[quoted_string])*[not_a_quote_or_comment]*[comment]
        if( $ibuf =~ m/^(([^\'\"\!]*[\'\"][^\'\"]*[\'\"])*[^\'\"\!]*)(\s*\!.*)/ )
        {
            if( $debug ){ print "stripTrailingComment: closed |$1|$3|\n"; }
            $ibuf = $1;
        }
    }
    # first, remove everything except quotes and comment chars
    my $buf = $ibuf ;
    ##[NOTE: perl trickyness: remove all chars that arent quotes or comment
    ##       and count what is left]
    $buf =~ tr/\'\"\!//cd ;
    if( $openquote )
    { # make believe there is another quote at the beginning
        $buf = "\'$buf" ;
    }
    my $oldopenquote = $openquote ;
    # Now, figure out if there is still an open quote by removing all the
    # pairs of quotes, which may have a comment char
    $buf =~ s/^([\'\"]\!*[\'\"])*// ;
    if( $buf eq "" or $buf =~ m/^\!/ )
    {   # no quotes or a comment
        $openquote = 0;
    }
    elsif( $buf =~ m/^[\'\"]/ )
    {
        $openquote = 1;
    }
    else
    {
        die "internalerror: stripcomp.pm:stripTrailingComment(): bad quote count for openquote=$openquote and line=[$ibuf]\n";
    }
    ## Work-around for a bug in the preprocessor in g++ v3.3.x where it appends
    ## a single-quote (') to a line if it ends in an unmatched single-quote.
    ## This is allowed in Fortran because string constants can be continued
    ## across lines.  So append a comment with a quote in it so the quote isn't
    ## the last character and the quotes are paired.
    if( $openquote != $oldopenquote and $ibuf =~ m/[\'\"]\s*$/ )
    {
        $ibuf .= " !\' work around gnu cpp v3.3 bug" ;
    }
    if( $lineterm ){ $ibuf .= $lineterm ; }
    return ( $ibuf , $openquote );
}

### Perl modules are executed when they load, so they must return true.
1;
