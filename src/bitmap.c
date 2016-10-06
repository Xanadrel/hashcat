/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "memory.h"
#include "logging.h"
#include "bitmap.h"

static u32 generate_bitmaps (const u32 digests_cnt, const u32 dgst_size, const u32 dgst_shifts, char *digests_buf_ptr, const u32 dgst_pos0, const u32 dgst_pos1, const u32 dgst_pos2, const u32 dgst_pos3, const u32 bitmap_mask, const u32 bitmap_size, u32 *bitmap_a, u32 *bitmap_b, u32 *bitmap_c, u32 *bitmap_d, const u64 collisions_max)
{
  u64 collisions = 0;

  memset (bitmap_a, 0, bitmap_size);
  memset (bitmap_b, 0, bitmap_size);
  memset (bitmap_c, 0, bitmap_size);
  memset (bitmap_d, 0, bitmap_size);

  for (u32 i = 0; i < digests_cnt; i++)
  {
    u32 *digest_ptr = (u32 *) digests_buf_ptr;

    digests_buf_ptr += dgst_size;

    const u32 val0 = 1u << (digest_ptr[dgst_pos0] & 0x1f);
    const u32 val1 = 1u << (digest_ptr[dgst_pos1] & 0x1f);
    const u32 val2 = 1u << (digest_ptr[dgst_pos2] & 0x1f);
    const u32 val3 = 1u << (digest_ptr[dgst_pos3] & 0x1f);

    const u32 idx0 = (digest_ptr[dgst_pos0] >> dgst_shifts) & bitmap_mask;
    const u32 idx1 = (digest_ptr[dgst_pos1] >> dgst_shifts) & bitmap_mask;
    const u32 idx2 = (digest_ptr[dgst_pos2] >> dgst_shifts) & bitmap_mask;
    const u32 idx3 = (digest_ptr[dgst_pos3] >> dgst_shifts) & bitmap_mask;

    if (bitmap_a[idx0] & val0) collisions++;
    if (bitmap_b[idx1] & val1) collisions++;
    if (bitmap_c[idx2] & val2) collisions++;
    if (bitmap_d[idx3] & val3) collisions++;

    bitmap_a[idx0] |= val0;
    bitmap_b[idx1] |= val1;
    bitmap_c[idx2] |= val2;
    bitmap_d[idx3] |= val3;

    if (collisions >= collisions_max) return 0x7fffffff;
  }

  return collisions;
}

void bitmap_ctx_init (hashcat_ctx_t *hashcat_ctx)
{
  bitmap_ctx_t   *bitmap_ctx   = hashcat_ctx->bitmap_ctx;
  hashconfig_t   *hashconfig   = hashcat_ctx->hashconfig;
  hashes_t       *hashes       = hashcat_ctx->hashes;
  user_options_t *user_options = hashcat_ctx->user_options;

  bitmap_ctx->enabled = false;

  if (user_options->keyspace    == true) return;
  if (user_options->left        == true) return;
  if (user_options->opencl_info == true) return;
  if (user_options->show        == true) return;
  if (user_options->usage       == true) return;
  if (user_options->version     == true) return;

  bitmap_ctx->enabled = true;

  /**
   * generate bitmap tables
   */

  const u32 bitmap_shift1 = 5;
  const u32 bitmap_shift2 = 13;

  const u32 bitmap_min = user_options->bitmap_min;
  const u32 bitmap_max = user_options->bitmap_max;

  u32 *bitmap_s1_a = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s1_b = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s1_c = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s1_d = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s2_a = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s2_b = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s2_c = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));
  u32 *bitmap_s2_d = (u32 *) mymalloc ((1u << bitmap_max) * sizeof (u32));

  u32 bitmap_bits;
  u32 bitmap_nums;
  u32 bitmap_mask;
  u32 bitmap_size;

  for (bitmap_bits = bitmap_min; bitmap_bits < bitmap_max; bitmap_bits++)
  {
    if (user_options->quiet == false) log_info_nn ("Generating bitmap tables with %u bits...", bitmap_bits);

    bitmap_nums = 1u << bitmap_bits;

    bitmap_mask = bitmap_nums - 1;

    bitmap_size = bitmap_nums * sizeof (u32);

    if ((hashes->digests_cnt & bitmap_mask) == hashes->digests_cnt) break;

    if (generate_bitmaps (hashes->digests_cnt, hashconfig->dgst_size, bitmap_shift1, (char *) hashes->digests_buf, hashconfig->dgst_pos0, hashconfig->dgst_pos1, hashconfig->dgst_pos2, hashconfig->dgst_pos3, bitmap_mask, bitmap_size, bitmap_s1_a, bitmap_s1_b, bitmap_s1_c, bitmap_s1_d, hashes->digests_cnt / 2) == 0x7fffffff) continue;
    if (generate_bitmaps (hashes->digests_cnt, hashconfig->dgst_size, bitmap_shift2, (char *) hashes->digests_buf, hashconfig->dgst_pos0, hashconfig->dgst_pos1, hashconfig->dgst_pos2, hashconfig->dgst_pos3, bitmap_mask, bitmap_size, bitmap_s1_a, bitmap_s1_b, bitmap_s1_c, bitmap_s1_d, hashes->digests_cnt / 2) == 0x7fffffff) continue;

    break;
  }

  bitmap_nums = 1u << bitmap_bits;

  bitmap_mask = bitmap_nums - 1;

  bitmap_size = bitmap_nums * sizeof (u32);

  generate_bitmaps (hashes->digests_cnt, hashconfig->dgst_size, bitmap_shift1, (char *) hashes->digests_buf, hashconfig->dgst_pos0, hashconfig->dgst_pos1, hashconfig->dgst_pos2, hashconfig->dgst_pos3, bitmap_mask, bitmap_size, bitmap_s1_a, bitmap_s1_b, bitmap_s1_c, bitmap_s1_d, -1ul);
  generate_bitmaps (hashes->digests_cnt, hashconfig->dgst_size, bitmap_shift2, (char *) hashes->digests_buf, hashconfig->dgst_pos0, hashconfig->dgst_pos1, hashconfig->dgst_pos2, hashconfig->dgst_pos3, bitmap_mask, bitmap_size, bitmap_s2_a, bitmap_s2_b, bitmap_s2_c, bitmap_s2_d, -1ul);

  bitmap_ctx->bitmap_bits   = bitmap_bits;
  bitmap_ctx->bitmap_nums   = bitmap_nums;
  bitmap_ctx->bitmap_size   = bitmap_size;
  bitmap_ctx->bitmap_mask   = bitmap_mask;
  bitmap_ctx->bitmap_shift1 = bitmap_shift1;
  bitmap_ctx->bitmap_shift2 = bitmap_shift2;

  bitmap_ctx->bitmap_s1_a   = bitmap_s1_a;
  bitmap_ctx->bitmap_s1_b   = bitmap_s1_b;
  bitmap_ctx->bitmap_s1_c   = bitmap_s1_c;
  bitmap_ctx->bitmap_s1_d   = bitmap_s1_d;
  bitmap_ctx->bitmap_s2_a   = bitmap_s2_a;
  bitmap_ctx->bitmap_s2_b   = bitmap_s2_b;
  bitmap_ctx->bitmap_s2_c   = bitmap_s2_c;
  bitmap_ctx->bitmap_s2_d   = bitmap_s2_d;
}

void bitmap_ctx_destroy (hashcat_ctx_t *hashcat_ctx)
{
  bitmap_ctx_t *bitmap_ctx = hashcat_ctx->bitmap_ctx;

  if (bitmap_ctx->enabled == false) return;

  myfree (bitmap_ctx->bitmap_s1_a);
  myfree (bitmap_ctx->bitmap_s1_b);
  myfree (bitmap_ctx->bitmap_s1_c);
  myfree (bitmap_ctx->bitmap_s1_d);
  myfree (bitmap_ctx->bitmap_s2_a);
  myfree (bitmap_ctx->bitmap_s2_b);
  myfree (bitmap_ctx->bitmap_s2_c);
  myfree (bitmap_ctx->bitmap_s2_d);

  memset (bitmap_ctx, 0, sizeof (bitmap_ctx_t));
}
