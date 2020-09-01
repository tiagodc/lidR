/*
 ===============================================================================

PROGRAMMERS:

jean-romain.roussel.1@ulaval.ca  -  https://github.com/Jean-Romain/lidR

COPYRIGHT:

Copyright 2016-2019 Jean-Romain Roussel

This file is part of lidR R package.

lidR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>

===============================================================================
*/

/*
 * ======= LAS FUNCTIONS =========
 */

#include "LAS.h"

//[[Rcpp::export(rng = false)]]
LogicalVector C_lmf(S4 las, NumericVector ws, double min_height, bool circular, int ncpu)
{
  LAS pt(las, ncpu);
  pt.filter_local_maxima(ws, min_height, circular);
  return Rcpp::wrap(pt.filter);
}

// [[Rcpp::export(rng = false)]]
NumericVector C_smooth(S4 las, double size, int method, int shape, double sigma, int ncpu)
{
  LAS pt(las, ncpu);
  pt.z_smooth(size, method, shape, sigma);
  return Rcpp::wrap(pt.Z);
}

// [[Rcpp::export(rng = false)]]
LogicalVector C_highest(S4 las, S4 layout)
{
  LAS pt(las);
  pt.filter_with_grid(layout);
  return Rcpp::wrap(pt.filter);
}

// [[Rcpp::export(rng = false)]]
LogicalVector C_in_polygon(S4 las, std::string wkt, int ncpu)
{
  LAS pt(las, ncpu);
  pt.filter_in_polygon(wkt);
  return Rcpp::wrap(pt.filter);
}

// [[Rcpp::export(rng = false)]]
LogicalVector C_lasdetectshape(S4 las, int method, NumericVector th, int k, LogicalVector filter, int ncpu)
{
  LAS pt(las, ncpu);
  pt.new_filter(filter);
  pt.filter_shape(method, th, k);
  return Rcpp::wrap(pt.filter);
}

// [[Rcpp::export(rng = false)]]
IntegerVector C_Wing2015(S4 las, NumericVector neigh_radii, double low_int_thrsh, double uppr_int_thrsh, int pt_den_req, NumericMatrix BBPRthrsh_mat, int ncpu)
{
  LAS pt(las, ncpu);
  return pt.segment_snags(neigh_radii, low_int_thrsh, uppr_int_thrsh, pt_den_req, BBPRthrsh_mat);
}

// [[Rcpp::export(rng = false)]]
IntegerVector C_li2012(S4 las, double dt1, double dt2, double Zu, double R, double th_tree, double radius)
{
  LAS pt(las);
  return pt.segment_trees(dt1, dt2, Zu, R, th_tree, radius);
}

// [[Rcpp::export(rng = false)]]
LogicalVector C_pmf(S4 las, NumericVector ws, NumericVector th, LogicalVector filter)
{
  LAS pt(las);
  pt.new_filter(filter);
  pt.filter_progressive_morphology(ws, th);
  return Rcpp::wrap(pt.filter);
}

// [[Rcpp::export(rng = false)]]
NumericVector C_rasterize(S4 las, S4 layout, double subcircle = 0, int method = 1)
{
  LAS pt(las);
  return pt.rasterize(layout, subcircle, method);
}

// [[Rcpp::export]]
List C_point_metrics(S4 las, unsigned int k, double r, int nalloc, SEXP call, SEXP env, LogicalVector filter)
{
  LAS pt(las);
  pt.new_filter(filter);
  DataFrame data = as<DataFrame>(las.slot("data"));
  return pt.point_metrics(k, r, data, nalloc, call, env);
}

// [[Rcpp::export]]
NumericVector C_fast_knn_metrics(S4 las, unsigned int k, IntegerVector metrics, int cpu)
{
  LAS pt(las, cpu);
  return pt.fast_knn_metrics(k, metrics);
}

// [[Rcpp::export(rng = false)]]
IntegerVector C_lasrangecorrection(S4 las, DataFrame flightlines, double Rs, double f)
{
  LAS pt(las);
  pt.i_range_correction(flightlines, Rs, f);
  return Rcpp::wrap(pt.I);
}

// [[Rcpp::export(rng = false)]]
NumericVector C_lasrange(S4 las, DataFrame flightlines)
{
  LAS pt(las);
  return pt.compute_range(flightlines);
}

//[[Rcpp::export(rng = false)]]
LogicalVector C_local_maximum(S4 las, NumericVector ws, int ncpu)
{
  LAS pt(las, ncpu);
  pt.filter_local_maxima(ws);
  return Rcpp::wrap(pt.filter);
}


// [[Rcpp::export(rng = false)]]
int C_check_gpstime(NumericVector t, IntegerVector rn)
{
  // This function count looks at each pulse (point with same gpstime)
  // and eval if some return number appear more than once. In theory this
  // should never happen but we have seen in #327 that it might exist.

  if (t.size() != rn.size()) stop("Internal error in C_check_gpstime: inputs of different sizes."); // # nocov

  std::map<double, unsigned int> registry;
  std::pair<std::map<double, unsigned int>::iterator, bool> ret;

  for (int i = 0 ; i < t.size() ; i++)
  {
    ret = registry.insert(std::pair<double, unsigned int>(t[i],0));
    if (ret.second == true) // gpstime first insertion
    {
      // We set the bit to 1
      ret.first->second  = ret.first->second | (1 << rn[i]);
    }
    else
    {
      // Get the bit
      bool bit = (ret.first->second >> rn[i]) & 1;

      // If bit conflict set the bit 31 to 1
      if (bit)
        ret.first->second  = ret.first->second | (1 << 31);
      else
        ret.first->second  = ret.first->second | (1 << rn[i]);
    }
  }

  int sum = 0;
  for (auto it = registry.begin() ; it != registry.end() ; it++)
  {
    bool bit = (it->second >> 31) & 1;
    if (bit) sum++;
  }

  return sum;
}

/*
 * ======= FAST BASE FUNCTIONS =========
 */

// [[Rcpp::export(rng=false)]]
IntegerVector fast_table(IntegerVector x, int size = 5)
{
  IntegerVector tbl(size);

  for (IntegerVector::iterator it = x.begin(), end = x.end() ; it != end ; ++it)
  {
    if (*it <= size && *it > 0)
      tbl(*it-1)++;
  }

  return tbl;
}

// [[Rcpp::export(rng=false)]]
int fast_countequal(IntegerVector x, int t)
{
  return std::count(x.begin(), x.end(), t);
}

// [[Rcpp::export(rng=false)]]
int fast_countbelow(NumericVector x, double t)
{
  return std::count_if(x.begin(), x.end(), std::bind(std::less<double>(), std::placeholders::_1, t));
}

// [[Rcpp::export(rng=false)]]
void fast_quantization(NumericVector x, double scale, double offset)
{
  int X = 0;

  for (NumericVector::iterator it = x.begin(), end = x.end() ; it != end ; ++it)
  {
    X = std::round((*it - offset)/scale);
    *it = X * scale + offset;
  }

  return;
}

// [[Rcpp::export(rng=false)]]
int fast_countunquantized(NumericVector x, double scale, double offset)
{
  int X = 0;
  int k = 0;

  // 32 bits fix
  if (sizeof(void*) == 4)
  {
    for (NumericVector::iterator it = x.begin(), end = x.end() ; it != end ; ++it)
    {
      X = std::round((*it - offset)/scale);
      if (std::abs(*it - (X * scale + offset)) > 1e-9) k++;
    }
  }
  else
  {
    for (NumericVector::iterator it = x.begin(), end = x.end() ; it != end ; ++it)
    {
      X = std::round((*it - offset)/scale);
      if (*it != X * scale + offset) k++;
    }
  }

  return k;
}

// [[Rcpp::export(rng=false)]]
int fast_countover(NumericVector x, double t)
{
  return std::count_if(x.begin(), x.end(), std::bind(std::greater<double>(), std::placeholders::_1, t));
}



// [[Rcpp::export(rng=false)]]
NumericVector roundc(NumericVector x, int digit = 0)
{
  NumericVector y(x.length());
  NumericVector::iterator itx = x.begin();
  NumericVector::iterator ity = y.begin();

  for(itx = x.begin(), ity = y.begin() ; itx != x.end() ; ++itx, ++ity)
  {
    *ity = round(*itx);
  }

  return y;
}

/*
 * ======= ALGEBRA FUNCTIONS =========
 */

#include <RcppArmadillo.h>

// [[Rcpp::export(rng=false)]]
SEXP fast_eigen_values(arma::mat A)
{
  arma::mat coeff;
  arma::mat score;
  arma::vec latent;
  arma::princomp(coeff, score, latent, A);
  NumericMatrix eigenvalues = Rcpp::wrap(latent);
  NumericMatrix eigencoeff = Rcpp::wrap(coeff);
  return(List::create(_["eigen"] = eigenvalues, _["coeff"] = eigencoeff));
}

/*
 * ======= BINARY SEARCH TREE FUNCTIONS =========
 */

#include "GridPartition.h"
#include "Progress.h"

using namespace lidR;

typedef lidR::GridPartition SpatialIndex;

// [[Rcpp::export(rng = false)]]
Rcpp::List C_knn(NumericVector X, NumericVector Y, NumericVector x, NumericVector y, int k, int ncpu)
{
  unsigned int n = x.length();
  IntegerMatrix knn_idx(n, k);
  NumericMatrix knn_dist(n, k);

  SpatialIndex tree(X,Y);

  #pragma omp parallel for num_threads(ncpu)
  for(unsigned int i = 0 ; i < n ; i++)
  {
    Point pt(x[i], y[i]);
    std::vector<PointXYZ*> pts;
    tree.knn(pt, k, pts);

    #pragma omp critical
    {
      for (unsigned int j = 0 ; j < pts.size() ; j++)
      {
        knn_idx(i, j)  = pts[j]->id + 1;

        double dx = pts[j]->x - x[i];
        double dy = pts[j]->y - y[i];

        knn_dist(i, j) = std::sqrt(dx*dx + dy*dy);
      }
    }
  }

  return Rcpp::List::create(Rcpp::Named("nn.idx") = knn_idx, Rcpp::Named("nn.dist") = knn_dist);
}

// [[Rcpp::export(rng = false)]]
NumericVector C_knnidw(NumericVector X, NumericVector Y, NumericVector Z, NumericVector x, NumericVector y, int k, double p, double rmax, int ncpu)
{
  unsigned int n = x.length();
  NumericVector iZ(n, NA_REAL);

  SpatialIndex tree(X,Y);
  Progress pb(n, "Inverse distance weighting: ");

  bool abort = false;

  #pragma omp parallel for num_threads(ncpu)
  for(unsigned int i = 0 ; i < n ; i++)
  {
    if (abort) continue;
    if (pb.check_interrupt()) abort = true;
    pb.increment();

    Point pt(x[i], y[i]);
    std::vector<PointXYZ*> pts;
    tree.knn(pt, k, rmax, pts);

    double sum_zw = 0;
    double sum_w  = 0;

    for (unsigned int j = 0 ; j < pts.size() ; j++)
    {
      double dx = pts[j]->x - x[i];
      double dy = pts[j]->y - y[i];
      double d  = std::sqrt(dx*dx + dy*dy);
      double w;
      double z = Z[pts[j]->id];

      if (d > 0)
      {
        w = 1/pow(d,p);
        sum_zw += z*w;
        sum_w  += w;
      }
      else
      {
        sum_zw = z;
        sum_w  = 1;
        break;
      }
    }

    #pragma omp critical
    {
      iZ(i) = sum_zw/sum_w;
    }
  }

  if (abort) throw Rcpp::internal::InterruptedException();

  return iZ;
}

// [[Rcpp::export(rng = false)]]
IntegerVector C_count_in_disc(NumericVector X, NumericVector Y, NumericVector x, NumericVector y, double radius, int ncpu)
{
  unsigned int n = x.length();
  IntegerVector output(n);

  SpatialIndex tree(X,Y);

  #pragma omp parallel for num_threads(ncpu)
  for(unsigned int i = 0 ; i < n ; i++)
  {
    Circle disc(x[i], y[i], radius);
    std::vector<PointXYZ*> pts;
    tree.lookup(disc, pts);

    #pragma omp critical
    {
      output[i] = pts.size();
    }
  }

  return output;
}


// Only for unit tests

// [[Rcpp::export(rng = false)]]
IntegerVector C_circle_lookup(NumericVector X, NumericVector Y, double x, double y, double r)
{
  std::vector<int> id;

  SpatialIndex tree(X,Y);
  std::vector<PointXYZ*> pts;
  Circle circ(x,y,r);
  tree.lookup(circ, pts);

  for (size_t j = 0 ; j < pts.size() ; j++)
    id.push_back(pts[j]->id + 1);

  return wrap(id);
}

// [[Rcpp::export(rng = false)]]
IntegerVector C_orectangle_lookup(NumericVector X, NumericVector Y, double x, double y, double w, double h, double angle)
{
  std::vector<int> id;

  double xmax = x+w/2;
  double ymax = y+h/2;
  double xmin = x-w/2;
  double ymin = y-h/2;

  SpatialIndex tree(X,Y);
  std::vector<PointXYZ*> pts;
  OrientedRectangle orect(xmin, xmax, ymin, ymax, angle);
  tree.lookup(orect, pts);

  for (size_t j = 0 ; j < pts.size() ; j++)
    id.push_back(pts[j]->id + 1);

  return wrap(id);
}


// [[Rcpp::export(rng = false)]]
IntegerVector C_knn3d_lookup(NumericVector X, NumericVector Y, NumericVector Z, double x, double y, double z, int k)
{
  std::vector<int> id;

  // Creation of a SpatialIndex
  SpatialIndex tree(X, Y, Z);

  PointXYZ p(x,y,z);
  std::vector<PointXYZ*> pts;
  tree.knn(p, k, pts);

  for (size_t j = 0 ; j < pts.size() ; j++)
    id.push_back(pts[j]->id + 1);

  return wrap(id);
}


