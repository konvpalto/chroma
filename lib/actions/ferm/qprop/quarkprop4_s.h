// $Id: quarkprop4_s.h,v 2.1 2005-12-15 04:03:27 edwards Exp $
/*! \file
 *  \brief Full quark propagator solver
 *
 *  Given a complete propagator as a source, this does all the inversions needed
 */

#ifndef __quarkprop4_s_h__
#define __quarkprop4_s_h__

#include "fermact.h"

namespace Chroma
{
  //! Given a complete propagator as a source, this does all the inversions needed
  /*! \ingroup qprop
   *
   * This routine is actually generic to all Wilson-like fermions
   *
   * \param q_sol    quark propagator ( Write )
   * \param q_src    source ( Read )
   * \param invParam inverter parameters ( Read )
   * \param ncg_had  number of CG iterations ( Write )
   */

  void quarkProp4(LatticeStaggeredPropagator& q_sol, 
		  XMLWriter& xml_out,
		  const LatticeStaggeredPropagator& q_src,
		  const StaggeredTypeFermAct< LatticeStaggeredFermion, multi1d<LatticeColorMatrix> >& S_f,
		  Handle<const ConnectState> state,
		  const InvertParam_t& invParam,
		  QuarkSpinType quarkSpinType,
		  int& ncg_had);

};
#endif
