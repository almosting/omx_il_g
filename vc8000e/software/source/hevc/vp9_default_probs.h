#ifndef _SOFT_VP9_DEFAULT_PROBS_H
#define _SOFT_VP9_DEFAULT_PROBS_H

#include "vp9_enums.h"

static const vp9_coeff_probs default_coef_probs_4x4[BLOCK_TYPES] = {
    {  /* block Type 0 */
     { /* Intra */
      {/* Coeff Band 0 */
       {195, 29, 183}, {84, 49, 136}, {8, 42, 71}},
      {/* Coeff Band 1 */
       {31, 107, 169}, {35, 99, 159}, {17, 82, 140}, {8, 66, 114}, {2, 44, 76},
       {1, 19, 32}},
      {/* Coeff Band 2 */
       {40, 132, 201}, {29, 114, 187}, {13, 91, 157}, {7, 75, 127}, {3, 58, 95},
       {1, 28, 47}},
      {/* Coeff Band 3 */
       {69, 142, 221}, {42, 122, 201}, {15, 91, 159}, {6, 67, 121}, {1, 42, 77},
       {1, 17, 31}},
      {/* Coeff Band 4 */
       {102, 148, 228}, {67, 117, 204}, {17, 82, 154}, {6, 59, 114},
       {2, 39, 75}, {1, 15, 29}},
      {/* Coeff Band 5 */
       {156, 57, 233}, {119, 57, 212}, {58, 48, 163}, {29, 40, 124},
       {12, 30, 81}, {3, 12, 31}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {191, 107, 226}, {124, 117, 204}, {25, 99, 155}},
      {/* Coeff Band 1 */
       {29, 148, 210}, {37, 126, 194}, {8, 93, 157}, {2, 68, 118}, {1, 39, 69},
       {1, 17, 33}},
      {/* Coeff Band 2 */
       {41, 151, 213}, {27, 123, 193}, {3, 82, 144}, {1, 58, 105}, {1, 32, 60},
       {1, 13, 26}},
      {/* Coeff Band 3 */
       {59, 159, 220}, {23, 126, 198}, {4, 88, 151}, {1, 66, 114}, {1, 38, 71},
       {1, 18, 34}},
      {/* Coeff Band 4 */
       {114, 136, 232}, {51, 114, 207}, {11, 83, 155}, {3, 56, 105},
       {1, 33, 65}, {1, 17, 34}},
      {/* Coeff Band 5 */
       {149, 65, 234}, {121, 57, 215}, {61, 49, 166}, {28, 36, 114},
       {12, 25, 76}, {3, 16, 42}}}},
    {  /* block Type 1 */
     { /* Intra */
      {/* Coeff Band 0 */
       {214, 49, 220}, {132, 63, 188}, {42, 65, 137}},
      {/* Coeff Band 1 */
       {85, 137, 221}, {104, 131, 216}, {49, 111, 192}, {21, 87, 155},
       {2, 49, 87}, {1, 16, 28}},
      {/* Coeff Band 2 */
       {89, 163, 230}, {90, 137, 220}, {29, 100, 183}, {10, 70, 135},
       {2, 42, 81}, {1, 17, 33}},
      {/* Coeff Band 3 */
       {108, 167, 237}, {55, 133, 222}, {15, 97, 179}, {4, 72, 135},
       {1, 45, 85}, {1, 19, 38}},
      {/* Coeff Band 4 */
       {124, 146, 240}, {66, 124, 224}, {17, 88, 175}, {4, 58, 122},
       {1, 36, 75}, {1, 18, 37}},
      {/* Coeff Band 5 */
       {141, 79, 241}, {126, 70, 227}, {66, 58, 182}, {30, 44, 136},
       {12, 34, 96}, {2, 20, 47}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {229, 99, 249}, {143, 111, 235}, {46, 109, 192}},
      {/* Coeff Band 1 */
       {82, 158, 236}, {94, 146, 224}, {25, 117, 191}, {9, 87, 149},
       {3, 56, 99}, {1, 33, 57}},
      {/* Coeff Band 2 */
       {83, 167, 237}, {68, 145, 222}, {10, 103, 177}, {2, 72, 131},
       {1, 41, 79}, {1, 20, 39}},
      {/* Coeff Band 3 */
       {99, 167, 239}, {47, 141, 224}, {10, 104, 178}, {2, 73, 133},
       {1, 44, 85}, {1, 22, 47}},
      {/* Coeff Band 4 */
       {127, 145, 243}, {71, 129, 228}, {17, 93, 177}, {3, 61, 124},
       {1, 41, 84}, {1, 21, 52}},
      {/* Coeff Band 5 */
       {157, 78, 244}, {140, 72, 231}, {69, 58, 184}, {31, 44, 137},
       {14, 38, 105}, {8, 23, 61}}}}};
static const vp9_coeff_probs default_coef_probs_8x8[BLOCK_TYPES] = {
    {  /* block Type 0 */
     { /* Intra */
      {/* Coeff Band 0 */
       {125, 34, 187}, {52, 41, 133}, {6, 31, 56}},
      {/* Coeff Band 1 */
       {37, 109, 153}, {51, 102, 147}, {23, 87, 128}, {8, 67, 101}, {1, 41, 63},
       {1, 19, 29}},
      {/* Coeff Band 2 */
       {31, 154, 185}, {17, 127, 175}, {6, 96, 145}, {2, 73, 114}, {1, 51, 82},
       {1, 28, 45}},
      {/* Coeff Band 3 */
       {23, 163, 200}, {10, 131, 185}, {2, 93, 148}, {1, 67, 111}, {1, 41, 69},
       {1, 14, 24}},
      {/* Coeff Band 4 */
       {29, 176, 217}, {12, 145, 201}, {3, 101, 156}, {1, 69, 111}, {1, 39, 63},
       {1, 14, 23}},
      {/* Coeff Band 5 */
       {57, 192, 233}, {25, 154, 215}, {6, 109, 167}, {3, 78, 118}, {1, 48, 69},
       {1, 21, 29}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {202, 105, 245}, {108, 106, 216}, {18, 90, 144}},
      {/* Coeff Band 1 */
       {33, 172, 219}, {64, 149, 206}, {14, 117, 177}, {5, 90, 141},
       {2, 61, 95}, {1, 37, 57}},
      {/* Coeff Band 2 */
       {33, 179, 220}, {11, 140, 198}, {1, 89, 148}, {1, 60, 104}, {1, 33, 57},
       {1, 12, 21}},
      {/* Coeff Band 3 */
       {30, 181, 221}, {8, 141, 198}, {1, 87, 145}, {1, 58, 100}, {1, 31, 55},
       {1, 12, 20}},
      {/* Coeff Band 4 */
       {32, 186, 224}, {7, 142, 198}, {1, 86, 143}, {1, 58, 100}, {1, 31, 55},
       {1, 12, 22}},
      {/* Coeff Band 5 */
       {57, 192, 227}, {20, 143, 204}, {3, 96, 154}, {1, 68, 112}, {1, 42, 69},
       {1, 19, 32}}}},
    {  /* block Type 1 */
     { /* Intra */
      {/* Coeff Band 0 */
       {212, 35, 215}, {113, 47, 169}, {29, 48, 105}},
      {/* Coeff Band 1 */
       {74, 129, 203}, {106, 120, 203}, {49, 107, 178}, {19, 84, 144},
       {4, 50, 84}, {1, 15, 25}},
      {/* Coeff Band 2 */
       {71, 172, 217}, {44, 141, 209}, {15, 102, 173}, {6, 76, 133},
       {2, 51, 89}, {1, 24, 42}},
      {/* Coeff Band 3 */
       {64, 185, 231}, {31, 148, 216}, {8, 103, 175}, {3, 74, 131}, {1, 46, 81},
       {1, 18, 30}},
      {/* Coeff Band 4 */
       {65, 196, 235}, {25, 157, 221}, {5, 105, 174}, {1, 67, 120}, {1, 38, 69},
       {1, 15, 30}},
      {/* Coeff Band 5 */
       {65, 204, 238}, {30, 156, 224}, {7, 107, 177}, {2, 70, 124}, {1, 42, 73},
       {1, 18, 34}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {225, 86, 251}, {144, 104, 235}, {42, 99, 181}},
      {/* Coeff Band 1 */
       {85, 175, 239}, {112, 165, 229}, {29, 136, 200}, {12, 103, 162},
       {6, 77, 123}, {2, 53, 84}},
      {/* Coeff Band 2 */
       {75, 183, 239}, {30, 155, 221}, {3, 106, 171}, {1, 74, 128}, {1, 44, 76},
       {1, 17, 28}},
      {/* Coeff Band 3 */
       {73, 185, 240}, {27, 159, 222}, {2, 107, 172}, {1, 75, 127}, {1, 42, 73},
       {1, 17, 29}},
      {/* Coeff Band 4 */
       {62, 190, 238}, {21, 159, 222}, {2, 107, 172}, {1, 72, 122}, {1, 40, 71},
       {1, 18, 32}},
      {/* Coeff Band 5 */
       {61, 199, 240}, {27, 161, 226}, {4, 113, 180}, {1, 76, 129}, {1, 46, 80},
       {1, 23, 41}}}}};
       
static const vp9_coeff_probs default_coef_probs_16x16[BLOCK_TYPES] = {
    {  /* block Type 0 */
     { /* Intra */
      {/* Coeff Band 0 */
       {7, 27, 153}, {5, 30, 95}, {1, 16, 30}},
      {/* Coeff Band 1 */
       {50, 75, 127}, {57, 75, 124}, {27, 67, 108}, {10, 54, 86}, {1, 33, 52},
       {1, 12, 18}},
      {/* Coeff Band 2 */
       {43, 125, 151}, {26, 108, 148}, {7, 83, 122}, {2, 59, 89}, {1, 38, 60},
       {1, 17, 27}},
      {/* Coeff Band 3 */
       {23, 144, 163}, {13, 112, 154}, {2, 75, 117}, {1, 50, 81}, {1, 31, 51},
       {1, 14, 23}},
      {/* Coeff Band 4 */
       {18, 162, 185}, {6, 123, 171}, {1, 78, 125}, {1, 51, 86}, {1, 31, 54},
       {1, 14, 23}},
      {/* Coeff Band 5 */
       {15, 199, 227}, {3, 150, 204}, {1, 91, 146}, {1, 55, 95}, {1, 30, 53},
       {1, 11, 20}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {19, 55, 240}, {19, 59, 196}, {3, 52, 105}},
      {/* Coeff Band 1 */
       {41, 166, 207}, {104, 153, 199}, {31, 123, 181}, {14, 101, 152},
       {5, 72, 106}, {1, 36, 52}},
      {/* Coeff Band 2 */
       {35, 176, 211}, {12, 131, 190}, {2, 88, 144}, {1, 60, 101}, {1, 36, 60},
       {1, 16, 28}},
      {/* Coeff Band 3 */
       {28, 183, 213}, {8, 134, 191}, {1, 86, 142}, {1, 56, 96}, {1, 30, 53},
       {1, 12, 20}},
      {/* Coeff Band 4 */
       {20, 190, 215}, {4, 135, 192}, {1, 84, 139}, {1, 53, 91}, {1, 28, 49},
       {1, 11, 20}},
      {/* Coeff Band 5 */
       {13, 196, 216}, {2, 137, 192}, {1, 86, 143}, {1, 57, 99}, {1, 32, 56},
       {1, 13, 24}}}},
    {  /* block Type 1 */
     { /* Intra */
      {/* Coeff Band 0 */
       {211, 29, 217}, {96, 47, 156}, {22, 43, 87}},
      {/* Coeff Band 1 */
       {78, 120, 193}, {111, 116, 186}, {46, 102, 164}, {15, 80, 128},
       {2, 49, 76}, {1, 18, 28}},
      {/* Coeff Band 2 */
       {71, 161, 203}, {42, 132, 192}, {10, 98, 150}, {3, 69, 109}, {1, 44, 70},
       {1, 18, 29}},
      {/* Coeff Band 3 */
       {57, 186, 211}, {30, 140, 196}, {4, 93, 146}, {1, 62, 102}, {1, 38, 65},
       {1, 16, 27}},
      {/* Coeff Band 4 */
       {47, 199, 217}, {14, 145, 196}, {1, 88, 142}, {1, 57, 98}, {1, 36, 62},
       {1, 15, 26}},
      {/* Coeff Band 5 */
       {26, 219, 229}, {5, 155, 207}, {1, 94, 151}, {1, 60, 104}, {1, 36, 62},
       {1, 16, 28}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {233, 29, 248}, {146, 47, 220}, {43, 52, 140}},
      {/* Coeff Band 1 */
       {100, 163, 232}, {179, 161, 222}, {63, 142, 204}, {37, 113, 174},
       {26, 89, 137}, {18, 68, 97}},
      {/* Coeff Band 2 */
       {85, 181, 230}, {32, 146, 209}, {7, 100, 164}, {3, 71, 121}, {1, 45, 77},
       {1, 18, 30}},
      {/* Coeff Band 3 */
       {65, 187, 230}, {20, 148, 207}, {2, 97, 159}, {1, 68, 116}, {1, 40, 70},
       {1, 14, 29}},
      {/* Coeff Band 4 */
       {40, 194, 227}, {8, 147, 204}, {1, 94, 155}, {1, 65, 112}, {1, 39, 66},
       {1, 14, 26}},
      {/* Coeff Band 5 */
       {16, 208, 228}, {3, 151, 207}, {1, 98, 160}, {1, 67, 117}, {1, 41, 74},
       {1, 17, 31}}}}};
static const vp9_coeff_probs default_coef_probs_32x32[BLOCK_TYPES] = {
    {  /* block Type 0 */
     { /* Intra */
      {/* Coeff Band 0 */
       {17, 38, 140}, {7, 34, 80}, {1, 17, 29}},
      {/* Coeff Band 1 */
       {37, 75, 128}, {41, 76, 128}, {26, 66, 116}, {12, 52, 94}, {2, 32, 55},
       {1, 10, 16}},
      {/* Coeff Band 2 */
       {50, 127, 154}, {37, 109, 152}, {16, 82, 121}, {5, 59, 85}, {1, 35, 54},
       {1, 13, 20}},
      {/* Coeff Band 3 */
       {40, 142, 167}, {17, 110, 157}, {2, 71, 112}, {1, 44, 72}, {1, 27, 45},
       {1, 11, 17}},
      {/* Coeff Band 4 */
       {30, 175, 188}, {9, 124, 169}, {1, 74, 116}, {1, 48, 78}, {1, 30, 49},
       {1, 11, 18}},
      {/* Coeff Band 5 */
       {10, 222, 223}, {2, 150, 194}, {1, 83, 128}, {1, 48, 79}, {1, 27, 45},
       {1, 11, 17}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {36, 41, 235}, {29, 36, 193}, {10, 27, 111}},
      {/* Coeff Band 1 */
       {85, 165, 222}, {177, 162, 215}, {110, 135, 195}, {57, 113, 168},
       {23, 83, 120}, {10, 49, 61}},
      {/* Coeff Band 2 */
       {85, 190, 223}, {36, 139, 200}, {5, 90, 146}, {1, 60, 103}, {1, 38, 65},
       {1, 18, 30}},
      {/* Coeff Band 3 */
       {72, 202, 223}, {23, 141, 199}, {2, 86, 140}, {1, 56, 97}, {1, 36, 61},
       {1, 16, 27}},
      {/* Coeff Band 4 */
       {55, 218, 225}, {13, 145, 200}, {1, 86, 141}, {1, 57, 99}, {1, 35, 61},
       {1, 13, 22}},
      {/* Coeff Band 5 */
       {15, 235, 212}, {1, 132, 184}, {1, 84, 139}, {1, 57, 97}, {1, 34, 56},
       {1, 14, 23}}}},
    {  /* block Type 1 */
     { /* Intra */
      {/* Coeff Band 0 */
       {181, 21, 201}, {61, 37, 123}, {10, 38, 71}},
      {/* Coeff Band 1 */
       {47, 106, 172}, {95, 104, 173}, {42, 93, 159}, {18, 77, 131},
       {4, 50, 81}, {1, 17, 23}},
      {/* Coeff Band 2 */
       {62, 147, 199}, {44, 130, 189}, {28, 102, 154}, {18, 75, 115},
       {2, 44, 65}, {1, 12, 19}},
      {/* Coeff Band 3 */
       {55, 153, 210}, {24, 130, 194}, {3, 93, 146}, {1, 61, 97}, {1, 31, 50},
       {1, 10, 16}},
      {/* Coeff Band 4 */
       {49, 186, 223}, {17, 148, 204}, {1, 96, 142}, {1, 53, 83}, {1, 26, 44},
       {1, 11, 17}},
      {/* Coeff Band 5 */
       {13, 217, 212}, {2, 136, 180}, {1, 78, 124}, {1, 50, 83}, {1, 29, 49},
       {1, 14, 23}}},
     { /* Inter */
      {/* Coeff Band 0 */
       {197, 13, 247}, {82, 17, 222}, {25, 17, 162}},
      {/* Coeff Band 1 */
       {126, 186, 247}, {234, 191, 243}, {176, 177, 234}, {104, 158, 220},
       {66, 128, 186}, {55, 90, 137}},
      {/* Coeff Band 2 */
       {111, 197, 242}, {46, 158, 219}, {9, 104, 171}, {2, 65, 125},
       {1, 44, 80}, {1, 17, 91}},
      {/* Coeff Band 3 */
       {104, 208, 245}, {39, 168, 224}, {3, 109, 162}, {1, 79, 124},
       {1, 50, 102}, {1, 43, 102}},
      {/* Coeff Band 4 */
       {84, 220, 246}, {31, 177, 231}, {2, 115, 180}, {1, 79, 134}, {1, 55, 77},
       {1, 60, 79}},
      {/* Coeff Band 5 */
       {43, 243, 240}, {8, 180, 217}, {1, 115, 166}, {1, 84, 121}, {1, 51, 67},
       {1, 16, 6}}}}};

const vp9_prob vp9_partition_probs
    [NUM_FRAME_TYPES][NUM_PARTITION_CONTEXTS][PARTITION_TYPES] =
        { /* 1 byte padding */
         {/* frame_type = keyframe */
          /* 8x8 -> 4x4 */
          {158, 97, 94, 0} /* a/l both not split */,
          {93, 24, 99, 0} /* a split, l not split */,
          {85, 119, 44, 0} /* l split, a not split */,
          {62, 59, 67, 0} /* a/l both split */,
          /* 16x16 -> 8x8 */
          {149, 53, 53, 0} /* a/l both not split */,
          {94, 20, 48, 0} /* a split, l not split */,
          {83, 53, 24, 0} /* l split, a not split */,
          {52, 18, 18, 0} /* a/l both split */,
          /* 32x32 -> 16x16 */
          {150, 40, 39, 0} /* a/l both not split */,
          {78, 12, 26, 0} /* a split, l not split */,
          {67, 33, 11, 0} /* l split, a not split */,
          {24, 7, 5, 0} /* a/l both split */,
          /* 64x64 -> 32x32 */
          {174, 35, 49, 0} /* a/l both not split */,
          {68, 11, 27, 0} /* a split, l not split */,
          {57, 15, 9, 0} /* l split, a not split */,
          {12, 3, 3, 0} /* a/l both split */
         },
         {/* frame_type = interframe */
          /* 8x8 -> 4x4 */
          {199, 122, 141, 0} /* a/l both not split */,
          {147, 63, 159, 0} /* a split, l not split */,
          {148, 133, 118, 0} /* l split, a not split */,
          {121, 104, 114, 0} /* a/l both split */,
          /* 16x16 -> 8x8 */
          {174, 73, 87, 0} /* a/l both not split */,
          {92, 41, 83, 0} /* a split, l not split */,
          {82, 99, 50, 0} /* l split, a not split */,
          {53, 39, 39, 0} /* a/l both split */,
          /* 32x32 -> 16x16 */
          {177, 58, 59, 0} /* a/l both not split */,
          {68, 26, 63, 0} /* a split, l not split */,
          {52, 79, 25, 0} /* l split, a not split */,
          {17, 14, 12, 0} /* a/l both split */,
          /* 64x64 -> 32x32 */
          {222, 34, 30, 0} /* a/l both not split */,
          {72, 16, 44, 0} /* a split, l not split */,
          {58, 32, 12, 0} /* l split, a not split */,
          {10, 7, 6, 0} /* a/l both split */
         }};


const struct NmvContext vp9_default_nmv_context = {
    {32, 64, 96},                 /* joints */
    {128, 128},                   /* sign */
    {{216}, {208}},               /* class0 */
    {{64, 96, 64}, {64, 96, 64}}, /* fp */
    {160, 160},                   /* class0_hp bit */
    {128, 128},                   /* hp */
    {{224, 144, 192, 168, 192, 176, 192, 198, 198, 245},
     {216, 128, 176, 160, 176, 176, 192, 198, 198, 208}}, /* class */
    {{{128, 128, 64}, {96, 112, 64}},
     {{128, 128, 64}, {96, 112, 64}}}, /* class0_fp */
    {{136, 140, 148, 160, 176, 192, 224, 234, 234, 240},
     {136, 140, 148, 160, 176, 192, 224, 234, 234, 240}}, /* bits */
};

const u8 vp9_kf_default_bmode_probs
    [VP9_INTRA_MODES][VP9_INTRA_MODES][VP9_INTRA_MODES - 1] = {
        {/* above = dc */
         {137, 30, 42, 148, 151, 207, 70, 52, 91} /* left = dc */,
         {92, 45, 102, 136, 116, 180, 74, 90, 100} /* left = v */,
         {73, 32, 19, 187, 222, 215, 46, 34, 100} /* left = h */,
         {91, 30, 32, 116, 121, 186, 93, 86, 94} /* left = d45 */,
         {72, 35, 36, 149, 68, 206, 68, 63, 105} /* left = d135 */,
         {73, 31, 28, 138, 57, 124, 55, 122, 151} /* left = d117 */,
         {67, 23, 21, 140, 126, 197, 40, 37, 171} /* left = d153 */,
         {86, 27, 28, 128, 154, 212, 45, 43, 53} /* left = d27 */,
         {74, 32, 27, 107, 86, 160, 63, 134, 102} /* left = d63 */,
         {59, 67, 44, 140, 161, 202, 78, 67, 119} /* left = tm */
        },
        {/* above = v */
         {63, 36, 126, 146, 123, 158, 60, 90, 96} /* left = dc */,
         {43, 46, 168, 134, 107, 128, 69, 142, 92} /* left = v */,
         {44, 29, 68, 159, 201, 177, 50, 57, 77} /* left = h */,
         {58, 38, 76, 114, 97, 172, 78, 133, 92} /* left = d45 */,
         {46, 41, 76, 140, 63, 184, 69, 112, 57} /* left = d135 */,
         {38, 32, 85, 140, 46, 112, 54, 151, 133} /* left = d117 */,
         {39, 27, 61, 131, 110, 175, 44, 75, 136} /* left = d153 */,
         {52, 30, 74, 113, 130, 175, 51, 64, 58} /* left = d27 */,
         {47, 35, 80, 100, 74, 143, 64, 163, 74} /* left = d63 */,
         {36, 61, 116, 114, 128, 162, 80, 125, 82} /* left = tm */
        },
        {/* above = h */
         {82, 26, 26, 171, 208, 204, 44, 32, 105} /* left = dc */,
         {55, 44, 68, 166, 179, 192, 57, 57, 108} /* left = v */,
         {42, 26, 11, 199, 241, 228, 23, 15, 85} /* left = h */,
         {68, 42, 19, 131, 160, 199, 55, 52, 83} /* left = d45 */,
         {58, 50, 25, 139, 115, 232, 39, 52, 118} /* left = d135 */,
         {50, 35, 33, 153, 104, 162, 64, 59, 131} /* left = d117 */,
         {44, 24, 16, 150, 177, 202, 33, 19, 156} /* left = d153 */,
         {55, 27, 12, 153, 203, 218, 26, 27, 49} /* left = d27 */,
         {53, 49, 21, 110, 116, 168, 59, 80, 76} /* left = d63 */,
         {38, 72, 19, 168, 203, 212, 50, 50, 107} /* left = tm */
        },
        {/* above = d45 */
         {103, 26, 36, 129, 132, 201, 83, 80, 93} /* left = dc */,
         {59, 38, 83, 112, 103, 162, 98, 136, 90} /* left = v */,
         {62, 30, 23, 158, 200, 207, 59, 57, 50} /* left = h */,
         {67, 30, 29, 84, 86, 191, 102, 91, 59} /* left = d45 */,
         {60, 32, 33, 112, 71, 220, 64, 89, 104} /* left = d135 */,
         {53, 26, 34, 130, 56, 149, 84, 120, 103} /* left = d117 */,
         {53, 21, 23, 133, 109, 210, 56, 77, 172} /* left = d153 */,
         {77, 19, 29, 112, 142, 228, 55, 66, 36} /* left = d27 */,
         {61, 29, 29, 93, 97, 165, 83, 175, 162} /* left = d63 */,
         {47, 47, 43, 114, 137, 181, 100, 99, 95} /* left = tm */
        },
        {/* above = d135 */
         {69, 23, 29, 128, 83, 199, 46, 44, 101} /* left = dc */,
         {53, 40, 55, 139, 69, 183, 61, 80, 110} /* left = v */,
         {40, 29, 19, 161, 180, 207, 43, 24, 91} /* left = h */,
         {60, 34, 19, 105, 61, 198, 53, 64, 89} /* left = d45 */,
         {52, 31, 22, 158, 40, 209, 58, 62, 89} /* left = d135 */,
         {44, 31, 29, 147, 46, 158, 56, 102, 198} /* left = d117 */,
         {35, 19, 12, 135, 87, 209, 41, 45, 167} /* left = d153 */,
         {55, 25, 21, 118, 95, 215, 38, 39, 66} /* left = d27 */,
         {51, 38, 25, 113, 58, 164, 70, 93, 97} /* left = d63 */,
         {47, 54, 34, 146, 108, 203, 72, 103, 151} /* left = tm */
        },
        {/* above = d117 */
         {64, 19, 37, 156, 66, 138, 49, 95, 133} /* left = dc */,
         {46, 27, 80, 150, 55, 124, 55, 121, 135} /* left = v */,
         {36, 23, 27, 165, 149, 166, 54, 64, 118} /* left = h */,
         {53, 21, 36, 131, 63, 163, 60, 109, 81} /* left = d45 */,
         {40, 26, 35, 154, 40, 185, 51, 97, 123} /* left = d135 */,
         {35, 19, 34, 179, 19, 97, 48, 129, 124} /* left = d117 */,
         {36, 20, 26, 136, 62, 164, 33, 77, 154} /* left = d153 */,
         {45, 18, 32, 130, 90, 157, 40, 79, 91} /* left = d27 */,
         {45, 26, 28, 129, 45, 129, 49, 147, 123} /* left = d63 */,
         {38, 44, 51, 136, 74, 162, 57, 97, 121} /* left = tm */
        },
        {/* above = d153 */
         {75, 17, 22, 136, 138, 185, 32, 34, 166} /* left = dc */,
         {56, 39, 58, 133, 117, 173, 48, 53, 187} /* left = v */,
         {35, 21, 12, 161, 212, 207, 20, 23, 145} /* left = h */,
         {56, 29, 19, 117, 109, 181, 55, 68, 112} /* left = d45 */,
         {47, 29, 17, 153, 64, 220, 59, 51, 114} /* left = d135 */,
         {46, 16, 24, 136, 76, 147, 41, 64, 172} /* left = d117 */,
         {34, 17, 11, 108, 152, 187, 13, 15, 209} /* left = d153 */,
         {51, 24, 14, 115, 133, 209, 32, 26, 104} /* left = d27 */,
         {55, 30, 18, 122, 79, 179, 44, 88, 116} /* left = d63 */,
         {37, 49, 25, 129, 168, 164, 41, 54, 148} /* left = tm */
        },
        {/* above = d27 */
         {82, 22, 32, 127, 143, 213, 39, 41, 70} /* left = dc */,
         {62, 44, 61, 123, 105, 189, 48, 57, 64} /* left = v */,
         {47, 25, 17, 175, 222, 220, 24, 30, 86} /* left = h */,
         {68, 36, 17, 106, 102, 206, 59, 74, 74} /* left = d45 */,
         {57, 39, 23, 151, 68, 216, 55, 63, 58} /* left = d135 */,
         {49, 30, 35, 141, 70, 168, 82, 40, 115} /* left = d117 */,
         {51, 25, 15, 136, 129, 202, 38, 35, 139} /* left = d153 */,
         {68, 26, 16, 111, 141, 215, 29, 28, 28} /* left = d27 */,
         {59, 39, 19, 114, 75, 180, 77, 104, 42} /* left = d63 */,
         {40, 61, 26, 126, 152, 206, 61, 59, 93} /* left = tm */
        },
        {/* above = d63 */
         {78, 23, 39, 111, 117, 170, 74, 124, 94} /* left = dc */,
         {48, 34, 86, 101, 92, 146, 78, 179, 134} /* left = v */,
         {47, 22, 24, 138, 187, 178, 68, 69, 59} /* left = h */,
         {56, 25, 33, 105, 112, 187, 95, 177, 129} /* left = d45 */,
         {48, 31, 27, 114, 63, 183, 82, 116, 56} /* left = d135 */,
         {43, 28, 37, 121, 63, 123, 61, 192, 169} /* left = d117 */,
         {42, 17, 24, 109, 97, 177, 56, 76, 122} /* left = d153 */,
         {58, 18, 28, 105, 139, 182, 70, 92, 63} /* left = d27 */,
         {46, 23, 32, 74, 86, 150, 67, 183, 88} /* left = d63 */,
         {36, 38, 48, 92, 122, 165, 88, 137, 91} /* left = tm */
        },
        {/* above = tm */
         {65, 70, 60, 155, 159, 199, 61, 60, 81} /* left = dc */,
         {44, 78, 115, 132, 119, 173, 71, 112, 93} /* left = v */,
         {39, 38, 21, 184, 227, 206, 42, 32, 64} /* left = h */,
         {58, 47, 36, 124, 137, 193, 80, 82, 78} /* left = d45 */,
         {49, 50, 35, 144, 95, 205, 63, 78, 59} /* left = d135 */,
         {41, 53, 52, 148, 71, 142, 65, 128, 51} /* left = d117 */,
         {40, 36, 28, 143, 143, 202, 40, 55, 137} /* left = d153 */,
         {52, 34, 29, 129, 183, 227, 42, 35, 43} /* left = d27 */,
         {42, 44, 44, 104, 105, 164, 64, 130, 80} /* left = d63 */,
         {43, 81, 53, 140, 169, 204, 68, 84, 72} /* left = tm */
        }};


static const vp9_prob default_intra_inter_p[INTRA_INTER_CONTEXTS] = {9,   102,
                                                                     187, 225};

static const vp9_prob default_comp_inter_p[COMP_INTER_CONTEXTS] = {
    239, 183, 119, 96, 41};

static const vp9_prob default_comp_ref_p[VP9_REF_CONTEXTS] = {50,  126, 123,
                                                          221, 226};

static const vp9_prob default_single_ref_p[VP9_REF_CONTEXTS][2] = {
    {33, 16}, {77, 74}, {142, 142}, {172, 170}, {238, 247}};

const vp9_prob vp9_default_tx_probs_32x32p
    [VP9_TX_SIZE_CONTEXTS][TX_SIZE_MAX - 1] = {{3, 136, 37, }, {5, 52, 13, }, };
const vp9_prob vp9_default_tx_probs_16x16p
    [VP9_TX_SIZE_CONTEXTS][TX_SIZE_MAX - 2] = {{20, 152, }, {15, 101, }, };
const vp9_prob vp9_default_tx_probs_8x8p[VP9_TX_SIZE_CONTEXTS][TX_SIZE_MAX - 3] =
    {{100, }, {66, }, };

const vp9_prob vp9_default_mbskip_probs[MBSKIP_CONTEXTS] = {192, 128, 64};

const vp9_prob vp9_switchable_interp_prob
    [VP9_SWITCHABLE_FILTERS + 1][VP9_SWITCHABLE_FILTERS - 1] = {
        {235, 162}, {36, 255}, {34, 3}, {149, 144}};


#endif
