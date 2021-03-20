//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/FactorialCache.cpp - Released 2020-12-17T15:46:35Z
// ----------------------------------------------------------------------------
// This file is part of the PixInsight Class Library (PCL).
// PCL is a multiplatform C++ framework for development of PixInsight modules.
//
// Copyright (c) 2003-2020 Pleiades Astrophoto S.L. All Rights Reserved.
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (https://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

#include <pcl/Math.h>

namespace pcl
{

// ----------------------------------------------------------------------------

const double FactorialCache::s_lut[ FactorialCache::s_cacheSize+1 ] =
{
   1.000000000000000e+000,
   1.000000000000000e+000,
   2.000000000000000e+000,
   6.000000000000000e+000,
   2.400000000000000e+001,
   1.200000000000000e+002,
   7.200000000000000e+002,
   5.040000000000000e+003,
   4.032000000000000e+004,
   3.628800000000000e+005,
   3.628800000000000e+006,
   3.991680000000000e+007,
   4.790016000000000e+008,
   6.227020800000000e+009,
   8.717829120000000e+010,
   1.307674368000000e+012,
   2.092278988800000e+013,
   3.556874280960000e+014,
   6.402373705728000e+015,
   1.216451004088320e+017,
   2.432902008176640e+018,
   5.109094217170944e+019,
   1.124000727777608e+021,
   2.585201673888498e+022,
   6.204484017332394e+023,
   1.551121004333099e+025,
   4.032914611266057e+026,
   1.088886945041835e+028,
   3.048883446117138e+029,
   8.841761993739701e+030,
   2.652528598121910e+032,
   8.222838654177922e+033,
   2.631308369336935e+035,
   8.683317618811886e+036,
   2.952327990396041e+038,
   1.033314796638614e+040,
   3.719933267899012e+041,
   1.376375309122634e+043,
   5.230226174666010e+044,
   2.039788208119744e+046,
   8.159152832478977e+047,
   3.345252661316380e+049,
   1.405006117752880e+051,
   6.041526306337383e+052,
   2.658271574788449e+054,
   1.196222208654802e+056,
   5.502622159812088e+057,
   2.586232415111682e+059,
   1.241391559253607e+061,
   6.082818640342675e+062,
   3.041409320171338e+064,
   1.551118753287382e+066,
   8.065817517094388e+067,
   4.274883284060025e+069,
   2.308436973392414e+071,
   1.269640335365828e+073,
   7.109985878048635e+074,
   4.052691950487722e+076,
   2.350561331282879e+078,
   1.386831185456899e+080,
   8.320987112741392e+081,
   5.075802138772248e+083,
   3.146997326038794e+085,
   1.982608315404440e+087,
   1.268869321858842e+089,
   8.247650592082472e+090,
   5.443449390774431e+092,
   3.647111091818868e+094,
   2.480035542436831e+096,
   1.711224524281413e+098,
   1.197857166996989e+100,
   8.504785885678623e+101,
   6.123445837688609e+103,
   4.470115461512685e+105,
   3.307885441519387e+107,
   2.480914081139540e+109,
   1.885494701666051e+111,
   1.451830920282859e+113,
   1.132428117820630e+115,
   8.946182130782979e+116,
   7.156945704626382e+118,
   5.797126020747370e+120,
   4.753643337012843e+122,
   3.945523969720660e+124,
   3.314240134565354e+126,
   2.817104114380551e+128,
   2.422709538367274e+130,
   2.107757298379528e+132,
   1.854826422573984e+134,
   1.650795516090846e+136,
   1.485715964481761e+138,
   1.352001527678403e+140,
   1.243841405464131e+142,
   1.156772507081642e+144,
   1.087366156656743e+146,
   1.032997848823906e+148,
   9.916779348709498e+149,
   9.619275968248213e+151,
   9.426890448883249e+153,
   9.332621544394417e+155,
   9.332621544394415e+157,
   9.425947759838360e+159,
   9.614466715035127e+161,
   9.902900716486180e+163,
   1.029901674514563e+166,
   1.081396758240291e+168,
   1.146280563734708e+170,
   1.226520203196138e+172,
   1.324641819451829e+174,
   1.443859583202493e+176,
   1.588245541522743e+178,
   1.762952551090244e+180,
   1.974506857221074e+182,
   2.231192748659814e+184,
   2.543559733472188e+186,
   2.925093693493016e+188,
   3.393108684451898e+190,
   3.969937160808721e+192,
   4.684525849754290e+194,
   5.574585761207606e+196,
   6.689502913449126e+198,
   8.094298525273443e+200,
   9.875044200833599e+202,
   1.214630436702533e+205,
   1.506141741511141e+207,
   1.882677176888926e+209,
   2.372173242880046e+211,
   3.012660018457659e+213
};

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF pcl/FactorialCache.cpp - Released 2020-12-17T15:46:35Z
