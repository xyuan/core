/*
 * Copyright (C) 2011 Scientific Computation Research Center
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef AWR_H
#define AWR_H

/** \page awr Adjoint Weighted Residuals
  * This is the main API for SCOREC's AWR library
  *
  * These functions provide capabilities to solve adjoint
  * boundary value problems using a specified functional
  * quantity of interest.
  */

/** \file awr.h */

namespace Teuchos { class ParameterList; }

namespace apf {
class Mesh;
class Field;
}

/** \namespace awr
  * \brief All AWR symbols */
namespace awr {

/** \brief increase polynomial order of field by one
  * \details only supported for linear solution 
  * fields at the moment */
apf::Field* enrichSolution(apf::Field* sol, const char* name);

/** \brief solve an adjoint boundary value problem
  * \details The parameter list must contain the sublists
  * 'Adjoint Problem', 'Quantity of Interest', and
  * 'Boundary Conditions' to completely define a problem */
void solveAdjoint(Teuchos::ParameterList& p, apf::Mesh* m);

}

#endif
