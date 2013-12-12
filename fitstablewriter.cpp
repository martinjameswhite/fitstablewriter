#include        <cmath>
#include        <cstdlib>
#include        <fstream>
#include        <sstream>
#include        <iomanip>
#include        <list>
#include        <vector>
#include	"fitstablewriter.h"



int	main(int argc, char **argv)
{

  FITStablewriter::FITStablewriter fw;

  std::vector<int>   ivec(5,666);
  std::vector<float> fvec(5,333);
  fw.add_comment("Hi there");
  fw.add_fvec("HALF",fvec);
  fw.add_ivec("EVIL",ivec);
  fw.write("example.fits");

  return(0);
}
