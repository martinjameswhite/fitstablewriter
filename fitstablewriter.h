#ifndef	_FITSWRITER_H_
#define	_FITSWRITER_H_

#include	<cmath>
#include	<ctime>
#include	<cstdlib>
#include	<iostream>
#include	<fstream>
#include	<sstream>
#include	<iomanip>
#include	<cctype>
#include	<cstring>
#include	<list>
#include	<vector>


// A simple C++ class to enable the writing of FITS binary table files.
// This code supports only the situation where a number of arrays of equal
// length need to be written to a binary table on HDU 1.
//
// Author:	Martin White	(UCB)


// Put everything in its own namespace.
namespace FITStablewriter {

// These file-scope quantities are defined here so they can be easily
// changed, e.g. if an MPI version of the code is wanted.
inline void     myexit(const int flag)
// A version of exit which can be easily replaced.
{
  std::cerr<<"myexit called with flag "<<flag<<std::endl;
  std::cout.flush();
  std::cerr.flush();
  exit(flag);
}



inline void     myexception(std::exception& e)
// A simple exception handler.
{
  std::cout<<"myexception called with "<<e.what()<<std::endl;
  std::cout.flush();
  std::cerr.flush();
  exit(666);
}




class	FITStablewriter {
private:
  bool					littleendian;
  long					bsize,itemlength;
  std::ofstream				fs;
  std::vector< std::string >		klist_i,klist_f,clist;
  std::vector< std::vector<int> >	ilist;
  std::vector< std::vector<float> >	flist;
  void swap4endian(char buffer[], const int size) {
    // Swaps the buffer from little- to big-endian assuming the field
    // is 4-bytes long, i.e. a 4-byte int or a 4-byte float.
    for (int i=0; i<4*size; i+=4) {
      char tmp[4];
      for (int j=0; j<4; j++)
        tmp[j]=buffer[i+3-j];
      for (int j=0; j<4; j++)
        buffer[i+j]=tmp[j];
    }
  }
  const std::string string2label(const char label[]) const {
    // Returns an 8-byte string, converted to uppercase and
    // enclosed in single quotes.
    char ss[11];
    ss[0]=ss[9]='\'';ss[10]=0;
    int j=0;
    for (int i=1; i<9; ++i)
      if (label[j]!=0) {
        ss[i]=std::toupper(label[j]);
        ++j;
      }
      else
        ss[i]=' ';
    return(std::string(ss));
  }
  const std::string string2card(const std::string& str) const {
    // Converts a string to a "card image", which is an 80 byte ASCII
    // string padded with space and with no delimiters.
    const int MaxSize=80;
    if (str.size()>=MaxSize) {
      std::cerr<<"string2card has str.size too big."<<std::endl;
      myexit(1);
    }
    std::string ss(str);
    try {
      ss.append(std::string(MaxSize-str.size(),' '));
    } catch(std::exception& e) {myexception(e);}
    return(ss);
  }
  void write_card(const std::string line) {
    std::string card=string2card(line);
    fs.write((char *)&card[0],card.size());
    if (fs.fail()){std::cerr<<"Error writing to file."<<std::endl;myexit(1);}
    bsize += card.size();
  }
  void write_card(const char line[]) {
    write_card(std::string(line));
  }
  void write_space() {
    // Fills the current file with spaces up to a multiple of 2880 bytes.
    int pad = 2880 - bsize%2880;
    std::string ss(pad,' ');
    fs.write((char *)&ss[0],pad);
    if (fs.fail()){std::cerr<<"Error writing to file."<<std::endl;myexit(1);}
    bsize += pad;
  }
  void write_zero() {
    // Fills the current file with zeros up to a multiple of 2880 bytes.
    int pad = 2880 - bsize%2880;
    std::string ss(pad,'\0');
    fs.write((char *)&ss[0],pad);
    if (fs.fail()){std::cerr<<"Error writing to file."<<std::endl;myexit(1);}
    bsize += pad;
  }
  void write_primary_hdu() {
    // The 1st HDU which is empty, following convention.
    write_card("SIMPLE  =                    T");
    write_card("BITPIX  =                    8");
    write_card("NAXIS   =                    0");
    write_card("EXTEND  =                    T");
    write_card("END");
    write_space();
  }
  void write_comments() {
    // Writes the comments (if there are any).
    for (int i=0; i<clist.size(); ++i) {
      std::string str("COMMENT    ");
      try{str.append(clist[i]);}catch(std::exception& e){myexception(e);}
      write_card(str);
    }
  }
  void write_time() {
    // Write the time in UTC format.
    std::time_t now=std::time(0);
    std::tm* gmtime=std::gmtime(&now);
    std::stringstream ss;
    ss<<"COMMENT    UTC="
      <<1900+gmtime->tm_year<<"-"
      <<std::setw(2)<<std::setfill('0')<<1+gmtime->tm_mon<<"-"
      <<std::setw(2)<<std::setfill('0')<<gmtime->tm_mday<<"."
      <<std::setw(2)<<std::setfill('0')<<gmtime->tm_hour<<":"
      <<std::setw(2)<<std::setfill('0')<<gmtime->tm_min<<":"
      <<std::setw(2)<<std::setfill('0')<<gmtime->tm_sec;
    write_card(ss.str());
  }
  void write_types() {
    // Writes the label names and data types for each field to the header.
    int iaxis=1;
    for (int i=0; i<ilist.size(); ++i) {
      std::stringstream str;
      str<<"TTYPE"<<iaxis;
      str<<std::setw(8-str.str().size())<<" "<<"= "
         <<string2label(klist_i[i].c_str());
      write_card(str.str());
      str.str(""); str<<"TFORM"<<iaxis;
      str<<std::setw(8-str.str().size())<<" "<<"= "
         <<string2label("J");
      write_card(str.str());
      iaxis++;
    }
    for (int i=0; i<flist.size(); ++i) {
      std::stringstream str;
      str<<"TTYPE"<<iaxis;
      str<<std::setw(8-str.str().size())<<" "<<"= "
         <<string2label(klist_f[i].c_str());
      write_card(str.str());
      str.str(""); str<<"TFORM"<<iaxis;
      str<<std::setw(8-str.str().size())<<" "<<"= "
         <<string2label("E");
      write_card(str.str());
      iaxis++;
    }
  }
  void write_data_hdu() {
    // Writes the HDU containing the binary table with the data.
    long bytesperrow=klist_i.size()*sizeof(int)+klist_f.size()*sizeof(float);
    // Work out how many arrays in total we are writing.
    long ncols=klist_i.size() + klist_f.size();
    // Now write the header...
    std::stringstream ss;
    ss<<"XTENSION= "<<string2label("BINTABLE");
    write_card(ss.str());
    write_card("BITPIX  =  8");	// Required value.
    write_card("NAXIS   =  2");	// Required value.
    ss.str("");  ss<<"NAXIS1  =  "<<bytesperrow;
    write_card(ss.str());	// Number of BYTES per row.
    ss.str("");  ss<<"NAXIS2  =  "<<itemlength;
    write_card(ss.str());	// Number of rows.
    write_card("PCOUNT  =  0");	// No varying arrays.
    write_card("GCOUNT  =  1");	// Required value.
    ss.str("");  ss<<"TFIELDS =  "<<ncols;
    write_card(ss.str());	// Number of columns.
    write_time();
    write_comments();
    write_types();
    write_card("END");
    write_space();
  }
  void write_data() {
    // Writes the data to the file.
    // This has to be done transposed.
    // The write buffering is done explicitly here.
    const int BufSize=262144;	// Should be divisible by 8.
    int nstore;
    std::vector<char> buffer;
    try {
      buffer.resize(BufSize);
    } catch(std::exception& e) {myexception(e);}
    nstore=0;
    for (long j=0; j<itemlength; ++j) {
      for (int i=0; i<ilist.size(); ++i) {
        if (nstore+sizeof(int)>BufSize) {
          fs.write((char *)&buffer[0],nstore);
          if (fs.fail()){std::cerr<<"Error writing data."<<std::endl;myexit(1);}
          nstore=0;
        }
        std::memcpy(&buffer[nstore],&ilist[i][j],sizeof(int));
        nstore+= sizeof(int);
        bsize += sizeof(int);
      }
      for (int i=0; i<flist.size(); ++i) {
        if (nstore+sizeof(float)>BufSize) {
          fs.write((char *)&buffer[0],nstore);
          if (fs.fail()){std::cerr<<"Error writing data."<<std::endl;myexit(1);}
          nstore=0;
        }
        std::memcpy(&buffer[nstore],&flist[i][j],sizeof(float));
        nstore+= sizeof(float);
        bsize += sizeof(float);
      }
    }
    if (nstore>0) {
      fs.write((char *)&buffer[0],nstore);
      if (fs.fail()){std::cerr<<"Error writing data."<<std::endl;myexit(1);}
    }
  }
public:
  FITStablewriter() {
    // Just initialize the fields.
    klist_i.clear();
    klist_f.clear();
    clist.clear();
    ilist.clear();
    flist.clear();
    bsize=itemlength=0;
    // Work out whether we are big- or little-endian.
    long iflag=1;
    if (*(int *)&iflag==1)
      littleendian = true;
    else
      littleendian = false;
  }
  ~FITStablewriter() {}	// Do nothing.
  void add_ivec(const char keyname[], const std::vector<int>& ivec) {
    // Add an integer vector to the list of things to be written.
    // First check if the length is okay.  We could modify this
    // behavior to zero-pad all entries to the longest length.  For now
    // we require equal length.
    if (itemlength>0 && itemlength!=ivec.size()) {
      std::cerr<<"Item has different length."<<std::endl;
      myexit(666);
    }
    try{
      std::vector<int> buffer=ivec;
      if (littleendian) swap4endian((char *)&buffer[0],buffer.size());
      klist_i.push_back(std::string(keyname));
      ilist.push_back(buffer);	// Take a copy, in big-endian.
    } catch(std::exception& e) {myexception(e);}
    itemlength = ivec.size();
  }
  void add_fvec(const char keyname[], const std::vector<float>& fvec) {
    // Add a float vector to the list of things to be written.
    // First check if the length is okay.  We could modify this
    // behavior to zero-pad all entries to the longest length.  For now
    // we require equal length.
    if (itemlength>0 && itemlength!=fvec.size()) {
      std::cerr<<"Item has different length."<<std::endl;
      myexit(666);
    }
    try{
      std::vector<float> buffer=fvec;
      if (littleendian) swap4endian((char *)&buffer[0],buffer.size());
      klist_f.push_back(std::string(keyname));
      flist.push_back(buffer);	// Take a copy, in big-endian.
    } catch(std::exception& e) {myexception(e);}
    itemlength = fvec.size();
  }
  void add_comment(const std::string& comment) {
    // Adds a comment field.
    try {
      clist.push_back(comment);
    } catch(std::exception& e) {myexception(e);}
  }
  void add_comment(const char comment[]) {
    // Adds a comment field.
    add_comment(std::string(comment));
  }
  void write(const char fname[]) {
    // Writes the data to file.  The file is only open during this method.
    fs.open(fname,std::ios::binary);
    if (fs.fail()) {
      std::cerr<<"Unable to open "<<fname<<" for writing."<<std::endl;
      myexit(1);
    }
    // First write the header for HDU 0, which is empty.
    write_primary_hdu();
    // Now write the header for HDU 1, which will contain the data.
    write_data_hdu();
    // Now write the data and zero pad it before closing.
    write_data();
    write_zero();
    fs.close();
  }
};


}

#endif
