#ifndef hyb_int_h
#define hyb_int_h
#include <math.h>

#include "chromabase.h"
#include "update/field_state.h"
//#include "updates/abs_symp_updates.h"

#include <iostream>
using namespace QDP;
using namespace std;

//! I don't use this.
enum TrajLengthType_t {FIXED_LENGTH, EXPONENTIAL_LENGTH};

//! Abstract MD Integrator -- a'la Robert's previous attempts
template<typename P, typename Q>
class AbsHybInt {
public:
  //virtual destructor
  virtual ~AbsHybInt() {};

  // Do the integration: The XMLBufferWriter is for monitoring
  virtual void operator()(AbsFieldState<P,Q>& s, 
			  XMLWriter& mon_traj) const = 0;

  // Get step size
  virtual Real getStepSize(void) const = 0;

  // Get traj length
  virtual Real getTrajLength(void) const = 0;

  // Clone so we can copy through the base class
  virtual AbsHybInt* clone(void) const = 0;

};

//! Fermions
template<typename P, typename Q, typename Phi>
class AbsPFHybInt {
public:
  //virtual destructor
  virtual ~AbsPFHybInt() {};


  // Do the integration: The XMLBufferWriter is for monitoring
  virtual void operator()(AbsPFFieldState<P,Q,Phi>& s, 
			  XMLWriter& mon_traj) const = 0;

  // Get step size
  virtual Real getStepSize(void) const = 0;

  // Get traj length
  virtual Real getTrajLength(void) const = 0;

  

};


//! A Concrete Leapfrog for Pure Gauge Systems.
//  Templated on 

class PureGaugePQPLeapFrog : public AbsHybInt< multi1d<LatticeColorMatrix>, 
				multi1d<LatticeColorMatrix> >
{
 public: 
  // Virtual Destructor
  ~PureGaugePQPLeapFrog() {}

  PureGaugePQPLeapFrog(SymplecticUpdates<multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> >& symp_updates_,
		       Real delta_tau_, 
		       Real tau_) : symp_updates(symp_updates_.clone()), delta_tau(delta_tau_), tau(tau_) {}
  
  // Get at the leap P and leap Q
  // through a Symplectic Updates step.
  // Use Base Class...
  const SymplecticUpdates<multi1d<LatticeColorMatrix>,multi1d<LatticeColorMatrix> >& getSympUpdates(void) const {
    return *symp_updates;
  }

  // Get step size
  virtual Real getStepSize(void) const { return delta_tau; }

  // Get traj length
  virtual Real getTrajLength(void) const { return tau; }

  virtual void doReversedTraj(AbsFieldState<multi1d<LatticeColorMatrix>, 
			                    multi1d<LatticeColorMatrix> > &s,
			      XMLWriter& mon_traj) const
  {
    // DO Forward Traj
    (*this)(s, mon_traj);

    // Flip Momenta
    for(int mu=0; mu < Nd; mu++) { 
      s.getP()[mu] *= Double(-1);
    }

    // Do backward Traj
    (*this)(s, mon_traj);

    // Flip Momenta
    for(int mu=0; mu < Nd; mu++) { 
      s.getP()[mu] *= Double(-1);
    }
  }

  // This is the dumb one with no monitoring...
  // One can override...
  virtual void operator()(AbsFieldState<multi1d<LatticeColorMatrix>, 
			                multi1d<LatticeColorMatrix> > &s,
			  XMLWriter& mon_traj) const
  {
    const SymplecticUpdates<multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> >& leaps = getSympUpdates();
    Real dt = getStepSize();
    Real dtby2 = dt / Real(2);
    Real t = 0; 
    Real tau = getTrajLength();
   
    int halfPLeaps =0;
    int PLeaps = 0;
    int QLeaps = 0;

#if 0
    leaps.leapP(s, dtby2);
    halfPLeaps++;

    bool finished = false;
   
    while ( !finished ) { 
      leaps.leapQ(s, dt);
      QLeaps++;
      t += dt;

      // Check if we need to do the last half step.
      // I am not sure about this end of traj business below.
      // The only stupidity is that we can only compare 
      // tau and t within fuzz...
      if( toBool( fabs(tau - t) < dtby2 )) { 

	// tau = t so I need to finish up with a last half step
	leaps.leapP(s, dtby2);
	halfPLeaps++;
	finished = true;
      }
      else {
	
	// t < tau 
	leaps.leapP(s, dt);
	PLeaps++;
	finished = false;
      }
    
    }

    push(mon_traj, "PQPLeapFrogTraj");
    write(mon_traj, "tau", tau);
    write(mon_traj, "dt", dt);
    write(mon_traj, "halfPLeaps", halfPLeaps);
    write(mon_traj, "PLeaps", PLeaps);
    write(mon_traj, "QLeaps", QLeaps);
    pop(mon_traj);
    
#endif
    bool finished = false;
    while ( ! finished ) {
      leaps.leapP(s, dtby2 );
      leaps.leapQ(s, dt    );
      leaps.leapP(s, dtby2 );
      t += dt;
      if( toBool( fabs(tau - t) < dtby2 )) { 
	finished = true;
      }
    }
  }

  PureGaugePQPLeapFrog(const PureGaugePQPLeapFrog& f) : symp_updates(f.symp_updates->clone()), delta_tau(f.delta_tau), tau(f.tau) {}
  
  virtual PureGaugePQPLeapFrog* clone(void) const {
    return new PureGaugePQPLeapFrog(*this);
  }

protected:
  Handle<SymplecticUpdates<multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> > > symp_updates;
  Real delta_tau;
  Real tau;

};
#endif
