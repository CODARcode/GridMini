/*************************************************************************************

    Grid physics library, www.github.com/paboyle/Grid 

    Source file: ./benchmarks/Benchmark_su3.cc

    Copyright (C) 2015

Author: Peter Boyle <paboyle@ph.ed.ac.uk>
Author: Peter Boyle <peterboyle@Peters-MacBook-Pro-2.local>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    See the full license in the file "LICENSE" in the top level distribution directory
    *************************************************************************************/
    /*  END LEGAL */
#include <Grid/GridCore.h>
#include <Grid/GridStd.h>
using namespace std;
using namespace Grid;
#ifdef OMPTARGET_UVM
#pragma omp requires unified_shared_memory
#endif
int main (int argc, char ** argv)
{
  Grid_init(&argc,&argv);

#define LMAX (48)
#define LMIN (8)
#define LADD (8)
int64_t Nwarm=50;
int64_t Nloop=1000;
 
  Coordinate simd_layout = GridDefaultSimd(Nd,vComplex::Nsimd());
  std::cout<<GridLogMessage << "Grid simd_layout" << simd_layout << std::endl;
  Coordinate mpi_layout  = GridDefaultMpi();

  int64_t threads = GridThread::GetThreads();
  std::cout<<GridLogMessage << "Grid is setup to use "<<threads<<" CPU threads"<<std::endl;
  std::cout<<GridLogMessage << "Grid is setup to use "<<gpu_threads<<" GPU threads"<<std::endl;
  std::cout<<GridLogMessage << "Grid is setup to use "<<Nd<<" dimensions"<<std::endl;

#if 0
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  x= x*y"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;

  for(int lat=LMIN;lat<=LMAX;lat+=LADD){

      Coordinate latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];
      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);

      for(int64_t i=0;i<Nwarm;i++){
	x=x*y;
      }
      double start=usecond();
      for(int64_t i=0;i<Nloop;i++){
	x=x*y;
      }
      double stop=usecond();
      double time = (stop-start)/Nloop*1000.0;
      std::cout<<"time: "<<time<<std::endl; 
      double bytes=3.0*vol*Nc*Nc*sizeof(Complex);
      double footprint=2.0*vol*Nc*Nc*sizeof(Complex);
      double flops=Nc*Nc*(6.0+8.0+8.0)*vol;
      std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<footprint<<"    \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;

    }

#endif

  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  z= x*y"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;

  for(int lat=LMIN;lat<=LMAX;lat+=LADD){

      Coordinate latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];

      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);

      auto xv=x.View();
      auto yv=y.View();
      auto zv=z.View();
#ifdef SPOT_CHECK 
      LatticeColourMatrix zref(&Grid); 
      auto zref_v=zref.View();
      
      std::cout<<"SPOT_CHECK mode...."<<std::endl;
     
      //CPU calculation
      printf("=====Beginning reference CPU calculations=====\n");
      for(int64_t s=0;s<vol;s++) {
        zref_v[s]=xv[s]*yv[s];
      }
      printf("=====End reference CPU calculations=====\n");
     
      
      printf("=====Beginning GPU calculations=====\n");
      //expression template calculation if enabled
      z=x*y;
      printf("=====End GPU calculations=====\n");
      
     LatticeColourMatrix diff(&Grid);
     diff=z-zref;
     auto dv=diff.View();
     int s=SPOT_CHECK; 
     if (s >= vol) {
	     std::cout<<"Spot check failed; index out of bound"<<std::endl;
     	     return -1;
     }
     std::cout<<"s="<<s<<": CPU-GPU = "<<dv[s]<<std::endl;
     std::cout<<"s="<<s<<": GOT:" <<zv[s]<<" \n EXPECTED: "<<zref_v[s]<<std::endl;
     

#endif

#if defined (OMPTARGET)  && defined (OMPTARGET_MAP) 
     #pragma omp target enter data map(alloc:zv._odata[ :zv.size()]) \
                                    map(to:xv._odata[ :xv.size()]) \
                                    map(to:yv._odata[ :yv.size()])

      for(int64_t i=0;i<Nwarm;i++){
     #pragma omp target teams distribute parallel for thread_limit(gpu_threads)
      for(int64_t s=0;s<vol;s++) {
        zv[s]=xv[s]*yv[s];
      }
      }


      double start=usecond();
      for(int64_t i=0;i<Nloop;i++){
      #pragma omp target teams distribute parallel for thread_limit(gpu_threads)
      for(int64_t s=0;s<vol;s++) {
        zv[s]=xv[s]*yv[s];
      }
      }
      double stop=usecond();
       #pragma omp target exit data map (from:zv._odata[ :zv.size()])
       #pragma omp target exit data map (delete:yv._odata[ :yv.size()])
       #pragma omp target exit data map (delete:xv._odata[ :xv.size()])
#else
      for(int64_t i=0;i<Nwarm;i++){
	z=x*y;
      }


      double start=usecond();
      for(int64_t i=0;i<Nloop;i++){
	z=x*y;
      }
      double stop=usecond();
#endif
      double time = (stop-start)/Nloop*1000.0;
      
      double bytes=3*vol*Nc*Nc*sizeof(Complex);
      double flops=Nc*Nc*(6+8+8)*vol;
      std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<bytes<<"    \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;

  }

#if 0
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  mult(z,x,y)"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;


  for(int lat=LMIN;lat<=LMAX;lat+=LADD){

      Coordinate latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];

      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);

      for(int64_t i=0;i<Nwarm;i++){
	mult(z,x,y);
      }
      double start=usecond();
      for(int64_t i=0;i<Nloop;i++){
	mult(z,x,y);
	//	mac(z,x,y);
      }
      double stop=usecond();
      double time = (stop-start)/Nloop*1000.0;
      
      double bytes=3*vol*Nc*Nc*sizeof(Complex);
      double flops=Nc*Nc*(6+8+8)*vol;
      std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<bytes<<"    \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;

    }

  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  mac(z,x,y)"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;

  for(int lat=LMIN;lat<=LMAX;lat+=LADD){

      Coordinate latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];

      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);

      for(int64_t i=0;i<Nwarm;i++){
	mac(z,x,y);
      }
      double start=usecond();
      for(int64_t i=0;i<Nloop;i++){
	mac(z,x,y);
      }
      double stop=usecond();
      double time = (stop-start)/Nloop*1000.0;
      
      double bytes=4*vol*Nc*Nc*sizeof(Complex);
      double flops=Nc*Nc*(8+8+8)*vol;
      std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<bytes<<"   \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;

    }

#endif 

#if 0
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  CovShiftForward(z,x,y)"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;

  for(int lat=LMIN;lat<=LMAX;lat+=LADD){

      std::vector<int> latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];

      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);

      for(int mu=0;mu<4;mu++){
	      double start=usecond();
	      for(int64_t i=0;i<Nloop;i++){
	        z = PeriodicBC::CovShiftForward(x,mu,y);
	    }
	    double stop=usecond();
	    double time = (stop-start)/Nloop*1000.0;
	
	
	    double bytes=3*vol*Nc*Nc*sizeof(Complex);
	    double flops=Nc*Nc*(6+8+8)*vol;
	    std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<bytes<<"   \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;
      }
  }

#endif
#if 0
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "= Benchmarking SU3xSU3  z= x * Cshift(y)"<<std::endl;
  std::cout<<GridLogMessage << "===================================================================================================="<<std::endl;
  std::cout<<GridLogMessage << "  L  "<<"\t\t"<<"bytes"<<"\t\t\t"<<"GB/s\t\t GFlop/s"<<std::endl;
  std::cout<<GridLogMessage << "----------------------------------------------------------"<<std::endl;

  for(int lat=LMIN;lat<=LMAX;lat+=LADD){
      std::vector<int> latt_size  ({lat*mpi_layout[0],lat*mpi_layout[1],lat*mpi_layout[2],lat*mpi_layout[3]});
      int64_t vol = latt_size[0]*latt_size[1]*latt_size[2]*latt_size[3];

      GridCartesian     Grid(latt_size,simd_layout,mpi_layout);
      GridParallelRNG          pRNG(&Grid);      pRNG.SeedFixedIntegers(std::vector<int>({45,12,81,9}));

      LatticeColourMatrix z(&Grid); random(pRNG,z);
      LatticeColourMatrix x(&Grid); random(pRNG,x);
      LatticeColourMatrix y(&Grid); random(pRNG,y);
      LatticeColourMatrix tmp(&Grid);

      for(int mu=0;mu<4;mu++){
	double tshift=0;
	double tmult =0;

	double start=usecond();
	for(int64_t i=0;i<Nloop;i++){
	  tshift-=usecond();
	  tmp = Cshift(y,mu,-1);
	  tshift+=usecond();
	  tmult-=usecond();
	  z   = x*tmp;
	  tmult+=usecond();
	}
	double stop=usecond();
	double time = (stop-start)/Nloop;
	tshift = tshift/Nloop;
	tmult  = tmult /Nloop;
	
	double bytes=3*vol*Nc*Nc*sizeof(Complex);
	double flops=Nc*Nc*(6+8+8)*vol;
	std::cout<<GridLogMessage<<std::setprecision(3) << "total us "<<time<<" shift "<<tshift <<" mult "<<tmult<<std::endl;
	time = time * 1000; // convert to NS for GB/s
	std::cout<<GridLogMessage<<std::setprecision(3) << lat<<"\t\t"<<bytes<<"   \t\t"<<bytes/time<<"\t\t" << flops/time<<std::endl;
      }
    }
#endif
  Grid_finalize();
}
