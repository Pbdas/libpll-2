/*
    Copyright (C) 2015 Diego Darriba, Tomas Flouri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact: Diego Darriba <Diego.Darriba@h-its.org>,
    Exelixis Lab, Heidelberg Instutute for Theoretical Studies
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/
#include "common.h"

#define N_STATES_AA 20
#define N_CAT_GAMMA 4
#define FLOAT_PRECISION 4

static double alpha = 0.5;
static unsigned int n_cat_gamma = N_CAT_GAMMA;
unsigned int params_indices[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int main(int argc, char * argv[])
{
  unsigned int j;
  double lk_score;
  unsigned int n_sites = 15;
  unsigned int n_tips = 5;
  double rate_cats[N_CAT_GAMMA];
  pll_operation_t * operations;
  int return_val;

  operations = (pll_operation_t *)malloc(4* sizeof(pll_operation_t));

  operations[0].parent_clv_index    = 5;
  operations[0].child1_clv_index    = 0;
  operations[0].child2_clv_index    = 1;
  operations[0].child1_matrix_index = 1;
  operations[0].child2_matrix_index = 1;
  operations[0].parent_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[0].child1_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[0].child2_scaler_index = PLL_SCALE_BUFFER_NONE;

  operations[1].parent_clv_index    = 6;
  operations[1].child1_clv_index    = 5;
  operations[1].child2_clv_index    = 2;
  operations[1].child1_matrix_index = 0;
  operations[1].child2_matrix_index = 1;
  operations[1].parent_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[1].child1_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[1].child2_scaler_index = PLL_SCALE_BUFFER_NONE;

  operations[2].parent_clv_index    = 7;
  operations[2].child1_clv_index    = 3;
  operations[2].child2_clv_index    = 4;
  operations[2].child1_matrix_index = 1;
  operations[2].child2_matrix_index = 1;
  operations[2].parent_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[2].child1_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[2].child2_scaler_index = PLL_SCALE_BUFFER_NONE;

  /* check attributes */
  unsigned int attributes = get_attributes(argc, argv);

  if (attributes & PLL_ATTRIB_LIMIT_MEMORY)
    skip_test();

  pll_partition_t * partition;
  partition = pll_partition_create(
                              n_tips,      /* numer of tips */
                              4,           /* clv buffers */
                              N_STATES_AA, /* number of states */
                              n_sites,     /* sequence length */
                              1,           /* different rate parameters */
                              2*n_tips-3,  /* probability matrices */
                              n_cat_gamma, /* gamma categories */
                              0,           /* scale buffers */
                              attributes
                              );          /* attributes */

  if (!partition)
  {
    printf("Error %d: %s\n", pll_errno, pll_errmsg);
    fatal("Fail creating partition");
  }

  double branch_lengths[4] = { 0.1, 0.2, 1, 1};
  unsigned int matrix_indices[4] = { 0, 1, 2, 3 };
  double * persite_lnl = (double *) malloc(n_sites * sizeof(double));
  double checksum;

  if (pll_compute_gamma_cats(alpha, n_cat_gamma, rate_cats, PLL_GAMMA_RATES_MEAN) == PLL_FAILURE)
  {
    printf("Error %d: %s\n", pll_errno, pll_errmsg);
    fatal("Fail computing gamma cats");
  }

  pll_set_frequencies(partition, 0, pll_aa_freqs_dayhoff);
  pll_set_subst_params(partition, 0, pll_aa_rates_dayhoff);

  return_val = PLL_SUCCESS;
  return_val &= pll_set_tip_states(partition, 0, pll_map_aa, "PIGLRVTLRRDRMWI");
  return_val &= pll_set_tip_states(partition, 1, pll_map_aa, "IQGMDITIVT-----");
  return_val &= pll_set_tip_states(partition, 2, pll_map_aa, "--AFALLQKIGMPFE");
  return_val &= pll_set_tip_states(partition, 3, pll_map_aa, "MDISIVT------TA");
  return_val &= pll_set_tip_states(partition, 4, pll_map_aa, "GLSEQTVFHEIDQDK");

  if (!return_val)
    fatal("Error setting tip states");

  pll_set_category_rates(partition, rate_cats);

  pll_update_prob_matrices(partition, params_indices, matrix_indices, branch_lengths, 4);
  pll_update_partials(partition, operations, 3);

  for (j = 0; j < 4; ++j)
  {
    printf ("[%d] P-matrix for branch length %f\n", j+1, branch_lengths[j]);
    pll_show_pmatrix(partition, j, FLOAT_PRECISION);
    printf ("\n");
  }

  /* show CLVs */
  printf ("[5] CLV 5: ");
  pll_show_clv(partition,5,PLL_SCALE_BUFFER_NONE,FLOAT_PRECISION+1);
  printf ("[6] CLV 6: ");
  pll_show_clv(partition,6,PLL_SCALE_BUFFER_NONE,FLOAT_PRECISION+1);
  printf ("[7] CLV 7: ");
  pll_show_clv(partition,7,PLL_SCALE_BUFFER_NONE,FLOAT_PRECISION+1);

  lk_score = pll_compute_edge_loglikelihood(partition,
                                            6,
                                            PLL_SCALE_BUFFER_NONE,
                                            7,
                                            PLL_SCALE_BUFFER_NONE,
                                            0,
                                            params_indices,
                                            persite_lnl);

  printf("\n");
  printf("inner-inner logL: %.6f\n",
          lk_score);
  printf("persite logL:     ");
  checksum = 0.0;
  for (int i=0; i<n_sites; i++)
  {
    checksum += persite_lnl[i];
    printf("%.7f  ", persite_lnl[i]);
  }
  printf("\n");
  printf("checksum logL:    %.6f\n",
          checksum);

  /* move to tip inner */

  operations[0].parent_clv_index    = 7;
  operations[0].child1_clv_index    = 6;
  operations[0].child2_clv_index    = 3;
  operations[0].child1_matrix_index = 0;
  operations[0].child2_matrix_index = 1;
  operations[0].parent_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[0].child1_scaler_index = PLL_SCALE_BUFFER_NONE;
  operations[0].child2_scaler_index = PLL_SCALE_BUFFER_NONE;

  pll_update_partials(partition, operations, 1);

assert(n_sites == partition->sites);
  lk_score = pll_compute_edge_loglikelihood(partition,
                                            7,
                                            PLL_SCALE_BUFFER_NONE,
                                            4,
                                            PLL_SCALE_BUFFER_NONE,
                                            1,
                                            params_indices,
                                            persite_lnl);

  printf("tip-inner logL:   %.6f\n",
          lk_score);
  printf("persite logL:     ");
  checksum = 0.0;
  for (int i=0; i<n_sites; i++)
  {
    checksum += persite_lnl[i];
    printf("%.7f  ", persite_lnl[i]);
  }
  printf("\n");
  printf("checksum logL:    %.6f\n",
          checksum);

  pll_partition_destroy(partition);
  free(persite_lnl);
  free(operations);

  return (0);
}
