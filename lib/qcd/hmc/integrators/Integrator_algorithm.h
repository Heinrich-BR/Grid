//--------------------------------------------------------------------
/*! @file Integrator_algorithm.h
 * @brief Declaration of classes for the Molecular Dynamics algorithms
 *
 * @author Guido Cossu
 */
//--------------------------------------------------------------------

#ifndef INTEGRATOR_ALG_INCLUDED
#define INTEGRATOR_ALG_INCLUDED


namespace Grid{
  namespace QCD{


    class MinimumNorm2{
      const double lambda = 0.1931833275037836;
    public:
      void step (LatticeLorentzColourMatrix& U, 
		 int level, std::vector<int>& clock,
		 Integrator<MinimumNorm2>* Integ){
	// level  : current level
	// fl     : final level
	// eps    : current step size

	int fl = Integ->as.size() -1;
	double eps = Integ->Params.stepsize;
	
	for(int l=0; l<=level; ++l) eps/= 2.0*Integ->Nrel[l];
	
	int fin = Integ->Nrel[0];
	for(int l=1; l<=level; ++l) fin*= 2.0*Integ->Nrel[l];
	fin = 3*Integ->Params.MDsteps*fin -1;
	
	
	for(int e=0; e<Integ->Nrel[level]; ++e){
	  
	  if(clock[level] == 0){    // initial half step 
	    Integ->update_P(U,level,lambda*eps);
	    ++clock[level];
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< clock[level] <<std::endl;
	  }
	  
	  if(level == fl){          // lowest level 
	    Integ->update_U(U,0.5*eps);
	    
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"U "<< (clock[level]+1) <<std::endl;
	  }else{                 // recursive function call 
	    step(U,level+1,clock, Integ);
	  }
	  
	  Integ->update_P(U,level,(1.0-2.0*lambda)*eps);
	  ++clock[level];
	  for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	  std::cout<<GridLogMessage<<"P "<< (clock[level]) <<std::endl;
	  
	  if(level == fl){          // lowest level 
	    Integ->update_U(U,0.5*eps);
	    
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"U "<< (clock[level]+1) <<std::endl;
	  }else{                 // recursive function call 
	    step(U,level+1,clock, Integ);
	  }    
	  
	  
	  if(clock[level] == fin){  // final half step
	    Integ->update_P(U,level,lambda*eps);
	    
	    ++clock[level];
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< clock[level] <<std::endl;
	  }else{                  // bulk step
	    Integ->update_P(U,level,lambda*2.0*eps);
	    
	    clock[level]+=2;
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< clock[level] <<std::endl;
	  }
	}
	
	
	
      }
      
    };
    
    class LeapFrog{
    public:
      void step (LatticeLorentzColourMatrix& U, 
		 int level, std::vector<int>& clock,
		 Integrator<LeapFrog>* Integ){
	// level  : current level
	// fl     : final level
	// eps    : current step size
	
	int fl = Integ->as.size() -1;
	double eps = Integ->Params.stepsize;
	
	// Get current level step size
	for(int l=0; l<=level; ++l) eps/= Integ->Nrel[l];
	
	int fin = 1;
	for(int l=0; l<=level; ++l) fin*= Integ->Nrel[l];
	fin = 2*Integ->Params.MDsteps*fin - 1;
	
	for(int e=0; e<Integ->Nrel[level]; ++e){
	  
	  if(clock[level] == 0){    // initial half step
	    Integ->update_P(U, level,eps/2.0);
	    ++clock[level];
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< 0.5*clock[level] <<std::endl;
	  }
	  if(level == fl){          // lowest level
	    Integ->update_U(U, eps);
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"U "<< 0.5*(clock[level]+1) <<std::endl;
	  }else{                 // recursive function call
	    step(U, level+1,clock, Integ);
	  }
	  if(clock[level] == fin){  // final half step
	    Integ->update_P(U, level,eps/2.0);
	    
	    ++clock[level];
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< 0.5*clock[level] <<std::endl;
	  }else{                  // bulk step
	    Integ->update_P(U, level,eps);
	    
	    clock[level]+=2;
	    for(int l=0; l<level;++l) std::cout<<GridLogMessage<<"   ";
	    std::cout<<GridLogMessage<<"P "<< 0.5*clock[level] <<std::endl;
	  }
	}




      }
    };


  }
}



#endif//INTEGRATOR_INCLUDED
