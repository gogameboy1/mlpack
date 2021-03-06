/**
 * @file dbscan_main.cpp
 * @author Ryan Curtin
 *
 * Implementation of program to run DBSCAN.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/util/mlpack_main.hpp>
#include <mlpack/core/tree/binary_space_tree.hpp>
#include <mlpack/core/tree/rectangle_tree.hpp>
#include <mlpack/core/tree/cover_tree.hpp>

#include "dbscan.hpp"

using namespace mlpack;
using namespace mlpack::range;
using namespace mlpack::dbscan;
using namespace mlpack::metric;
using namespace mlpack::tree;
using namespace mlpack::util;
using namespace std;

PROGRAM_INFO("DBSCAN clustering",
    "This program implements the DBSCAN algorithm for clustering using "
    "accelerated tree-based range search.  The type of tree that is used "
    "may be parameterized, or brute-force range search may also be used."
    "\n\n"
    "The input dataset to be clustered may be specified with the " +
    PRINT_PARAM_STRING("input") + " parameter; the radius of each range "
    "search may be specified with the " + PRINT_PARAM_STRING("epsilon") +
    " parameters, and the minimum number of points in a cluster may be "
    "specified with the " + PRINT_PARAM_STRING("min_size") + " parameter."
    "\n\n"
    "The " + PRINT_PARAM_STRING("assignments") + " and " +
    PRINT_PARAM_STRING("centroids") + " output parameters may be "
    "used to save the output of the clustering. " +
    PRINT_PARAM_STRING("assignments") + " contains the cluster assignments of "
    "each point, and " + PRINT_PARAM_STRING("centroids") + " contains the "
    "centroids of each cluster."
    "\n\n"
    "The range search may be controlled with the " +
    PRINT_PARAM_STRING("tree_type") + ", " +
    PRINT_PARAM_STRING("single_mode") + ", and " +
    PRINT_PARAM_STRING("naive") + " parameters.  " +
    PRINT_PARAM_STRING("tree_type") + " can control the type of tree used for "
    "range search; this can take a variety of values: 'kd', 'r', 'r-star', 'x',"
    " 'hilbert-r', 'r-plus', 'r-plus-plus', 'cover', 'ball'. The " +
    PRINT_PARAM_STRING("single_mode") + " parameter will force single-tree "
    "search (as opposed to the default dual-tree search), and '" +
    PRINT_PARAM_STRING("naive") + " will force brute-force range search."
    "\n\n"
    "An example usage to run DBSCAN on the dataset in " +
    PRINT_DATASET("input") + " with a radius of 0.5 and a minimum cluster size"
    " of 5 is given below:"
    "\n\n" +
    PRINT_CALL("dbscan", "input", "input", "epsilon", 0.5, "min_size", 5));

PARAM_MATRIX_IN_REQ("input", "Input dataset to cluster.", "i");
PARAM_UROW_OUT("assignments", "Output matrix for assignments of each "
    "point.", "a");
PARAM_MATRIX_OUT("centroids", "Matrix to save output centroids to.", "C");

PARAM_DOUBLE_IN("epsilon", "Radius of each range search.", "e", 1.0);
PARAM_INT_IN("min_size", "Minimum number of points for a cluster.", "m", 5);

PARAM_STRING_IN("tree_type", "If using single-tree or dual-tree search, the "
    "type of tree to use ('kd', 'r', 'r-star', 'x', 'hilbert-r', 'r-plus', "
    "'r-plus-plus', 'cover', 'ball').", "t", "kd");
PARAM_FLAG("single_mode", "If set, single-tree range search (not dual-tree) "
    "will be used.", "S");
PARAM_FLAG("naive", "If set, brute-force range search (not tree-based) "
    "will be used.", "N");

// Actually run the clustering, and process the output.
template<typename RangeSearchType>
void RunDBSCAN(RangeSearchType rs = RangeSearchType())
{
  if (CLI::HasParam("single_mode"))
    rs.SingleMode() = true;

  // Load dataset.
  arma::mat dataset = std::move(CLI::GetParam<arma::mat>("input"));

  const double epsilon = CLI::GetParam<double>("epsilon");
  const size_t minSize = (size_t) CLI::GetParam<int>("min_size");

  DBSCAN<RangeSearchType> d(epsilon, minSize, !CLI::HasParam("single_mode"),
      rs);

  // If possible, avoid the overhead of calculating centroids.
  arma::Row<size_t> assignments;
  if (CLI::HasParam("centroids"))
  {
    arma::mat centroids;

    d.Cluster(dataset, assignments, centroids);

    CLI::GetParam<arma::mat>("centroids") = std::move(centroids);
  }
  else
  {
    d.Cluster(dataset, assignments);
  }

  if (CLI::HasParam("assignments"))
    CLI::GetParam<arma::Row<size_t>>("assignments") = std::move(assignments);
}

void mlpackMain()
{
  RequireAtLeastOnePassed({ "assignments", "centroids" }, false,
      "no output will be saved");

  ReportIgnoredParam({{ "naive", true }}, "single_mode");

  RequireParamInSet<string>("tree_type", { "kd", "cover", "r", "r-star", "x",
      "hilbert-r", "r-plus", "r-plus-plus", "ball" }, true,
      "unknown tree type");

  // Fire off naive search if needed.
  if (CLI::HasParam("naive"))
  {
    RangeSearch<> rs(true);
    RunDBSCAN(rs);
  }

  const string treeType = CLI::GetParam<string>("tree_type");
  if (treeType == "kd")
    RunDBSCAN<RangeSearch<>>();
  else if (treeType == "cover")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, StandardCoverTree>>();
  else if (treeType == "r")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, RTree>>();
  else if (treeType == "r-star")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, RStarTree>>();
  else if (treeType == "x")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, XTree>>();
  else if (treeType == "hilbert-r")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, HilbertRTree>>();
  else if (treeType == "r-plus")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, RPlusTree>>();
  else if (treeType == "r-plus-plus")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, RPlusPlusTree>>();
  else if (treeType == "ball")
    RunDBSCAN<RangeSearch<EuclideanDistance, arma::mat, BallTree>>();
}
