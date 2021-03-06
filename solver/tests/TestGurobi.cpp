/**
 * @file TestGurobi.cpp
 * @author Brahayam Ponton (brahayam.ponton@tuebingen.mpg.de)
 * @license License BSD-3-Clause
 * @copyright Copyright 2018, Gurobi Optimization, LLC
 * @date 2019-10-07
 * 
 * @brief Formulate a classic optimization problem
 * 
 * This example formulates and solves the following simple QP model:
 *
 *    minimize    x + y + x^2 + x*y + y^2 + y*z + z^2
 *    subject to  x + 2 y + 3 z >= 4
 *                x +   y       >= 1
 *
 *  The example illustrates the use of dense matrices to store A and Q
 *  (and dense vectors for the other relevant data).  We don't recommend
 *  that you use dense matrices, but this example may be helpful if you
 *  already have your data in this format.
 */

#include <iostream>
#include <gurobi_c++.h>
#include <gtest/gtest.h>

#define PRECISION 0.01

  class GurobiTest : public ::testing::Test
  {
    protected:
      virtual void SetUp() {}
      virtual void TearDown() {}
  };

  template <typename Derived1, typename Derived2>
  void check_matrix(const Derived1& ref, const Derived2& val)
  {
    for (int i=0; i<ref.rows(); i++)
    {
  	  for (int j=0; j<ref.cols(); j++)
  	  {
        EXPECT_NEAR(ref(i,j), val(i,j), 0.01);
  	  }
    }
  }

  static bool
  dense_optimize(GRBEnv* env,
                 int     rows,
                 int     cols,
                 double* c,     /* linear portion of objective function */
                 double* Q,     /* quadratic portion of objective function */
                 double* A,     /* constraint matrix */
                 char*   sense, /* constraint senses */
                 double* rhs,   /* RHS vector */
                 double* lb,    /* variable lower bounds */
                 double* ub,    /* variable upper bounds */
                 char*   vtype, /* variable types (continuous, binary, etc.) */
                 double* solution,
                 double* objvalP)
  {
    GRBModel model = GRBModel(*env);
    int i, j;
    bool success = false;

    /* Add variables to the model */
    GRBVar* vars = model.addVars(lb, ub, NULL, vtype, NULL, cols);

    /* Populate A matrix */
    for (i = 0; i < rows; i++) {
      GRBLinExpr lhs = 0;
      for (j = 0; j < cols; j++)
        if (A[i*cols+j] != 0)
          lhs += A[i*cols+j]*vars[j];
      model.addConstr(lhs, sense[i], rhs[i]);
    }

    GRBQuadExpr obj = 0;
    for (j = 0; j < cols; j++)
      obj += c[j]*vars[j];
    for (i = 0; i < cols; i++)
      for (j = 0; j < cols; j++)
        if (Q[i*cols+j] != 0)
          obj += Q[i*cols+j]*vars[i]*vars[j];

    model.setObjective(obj);
    model.optimize();

    if (model.get(GRB_IntAttr_Status) == GRB_OPTIMAL) {
      *objvalP = model.get(GRB_DoubleAttr_ObjVal);
      for (i = 0; i < cols; i++)
        solution[i] = vars[i].get(GRB_DoubleAttr_X);
      success = true;
    }

    delete[] vars;
    return success;
  }

  TEST_F(GurobiTest, GurobiTest01)
  {
    GRBEnv* env = 0;
    try {
      env = new GRBEnv();
      env->set(GRB_IntParam_OutputFlag, 0);
      double c[] = {1, 1, 0};
      double  Q[3][3] = {{1, 1, 0}, {0, 1, 1}, {0, 0, 1}};
      double  A[2][3] = {{1, 2, 3}, {1, 1, 0}};
      char    sense[] = {'>', '>'};
      double  rhs[]   = {4, 1};
      double  lb[]    = {0, 0, 0};
      bool    success;
      double  objval, sol[3];

      success = dense_optimize(env, 2, 3, c, &Q[0][0], &A[0][0], sense, rhs, lb, NULL, NULL, sol, &objval);

      EXPECT_NEAR(0.571429, sol[0], PRECISION);
      EXPECT_NEAR(0.428571, sol[1], PRECISION);
      EXPECT_NEAR(0.857143, sol[2], PRECISION);
    } catch(GRBException e) {
      std::cout << "Error code = " << e.getErrorCode() << std::endl;
      std::cout << e.getMessage() << std::endl;
    } catch(...) {
      std::cout << "Exception during optimization" << std::endl;
    }

    delete env;
  }
