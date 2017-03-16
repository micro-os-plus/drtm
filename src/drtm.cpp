/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2017 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <drtm/drtm.h>

// ---------------------------------------------------------------------------

namespace drtm
{

// ---------------------------------------------------------------------------
// Local data structures.

// Non FP stack context, 17 words.
// Offsets in words, from SP up.

// Saved always by ARM.
// (17 optional padding/aligner)
// 16 xPSR (xPSR bit 9 = 1 if padded)
// 15 return address (PC, R15)
// 14 LR (R14)
// 13 R12
// 12 R3
// 11 R2
// 10 R1
//  9 R0

// Saved always by context switch handler.
// "stmdb %[r]!, {r4-r9,sl,fp,lr}"
//  8 EXC_RETURN (R14)
//  7 FP (R11)
//  6 SL (R10)
//  5 R9
//  4 R8
//  3 R7
//  2 R6
//  1 R5
//  0 R4 <-- new SP value

  /*
   <!DOCTYPE feature SYSTEM "gdb-target.dtd">
   <target version="1.0">
   <architecture>arm</architecture>
   <feature name="org.gnu.gdb.arm.m-profile">
   <reg name="r0" bitsize="32" regnum="0" type="uint32" group="general"/>
   <reg name="r1" bitsize="32" regnum="1" type="uint32" group="general"/>
   <reg name="r2" bitsize="32" regnum="2" type="uint32" group="general"/>
   <reg name="r3" bitsize="32" regnum="3" type="uint32" group="general"/>
   <reg name="r4" bitsize="32" regnum="4" type="uint32" group="general"/>
   <reg name="r5" bitsize="32" regnum="5" type="uint32" group="general"/>
   <reg name="r6" bitsize="32" regnum="6" type="uint32" group="general"/>
   <reg name="r7" bitsize="32" regnum="7" type="uint32" group="general"/>
   <reg name="r8" bitsize="32" regnum="8" type="uint32" group="general"/>
   <reg name="r9" bitsize="32" regnum="9" type="uint32" group="general"/>
   <reg name="r10" bitsize="32" regnum="10" type="uint32" group="general"/>
   <reg name="r11" bitsize="32" regnum="11" type="uint32" group="general"/>
   <reg name="r12" bitsize="32" regnum="12" type="uint32" group="general"/>
   <reg name="sp" bitsize="32" regnum="13" type="data_ptr" group="general"/>
   <reg name="lr" bitsize="32" regnum="14" type="uint32" group="general"/>
   <reg name="pc" bitsize="32" regnum="15" type="code_ptr" group="general"/>
   <reg name="xpsr" bitsize="32" regnum="25" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-system">
   <reg name="msp" bitsize="32" regnum="26" type="uint32" group="general"/>
   <reg name="psp" bitsize="32" regnum="27" type="uint32" group="general"/>
   <reg name="primask" bitsize="32" regnum="28" type="uint32" group="general"/>
   <reg name="basepri" bitsize="32" regnum="29" type="uint32" group="general"/>
   <reg name="faultmask" bitsize="32" regnum="30" type="uint32" group="general"/>
   <reg name="control" bitsize="32" regnum="31" type="uint32" group="general"/>
   </feature>
   </target>
   */

// This table encodes the offsets in the µOS++ non-VFP stack frame for the
// registers defined in the previous XML.
// Offsets are from the saved SP to each register, in 32-bits words.
//
// Special cases:
// -1 return as 0x00000000
// -2 SP should be taken from TCB, not from the context.
  static const register_offset_t cortex_m4_stack_offsets[] =
    {
    //
        9,// R0
        10, // R1
        11, // R2
        12, // R3
        0, // R4
        1, // R5
        2, // R6
        3, // R7
        4, // R8
        5, // R9
        6, // R10
        7, // R11
        13, // R12
        -2, // SP
        14, // LR
        15, // PC
        16, // XPSR

        -1, // MSP
        -1, // PSP
        -1, // PRIMASK
        -1, // BASEPRI
        -1, // FAULTMASK
        -1, // CONTROL
      };

  static const stack_info_t cortex_m4_stack_info =
    {
    //
        .in_registers = 16 + 1 + 1, // R0-R15 + EXC_RETURN, +XPSR
        .out_registers = 16 + 1, // R0-R15 + xPSR
        .offsets = cortex_m4_stack_offsets, //
        .offsets_size = sizeof(cortex_m4_stack_offsets)
            / sizeof(cortex_m4_stack_offsets[0]),
    //
      };

// FP stack context, 50 words.
// Offsets in words, from SP up.

// Saved always by ARM.
// (50 optional padding/aligner)
// 49 FPSCR
// 48 S15
// ...
// 34 S1
// 33 S0
// 32 xPSR (xPSR bit 9 = 1 if padded)
// 31 return address (PC, R15)
// 30 LR (R14)
// 29 R12
// 28 R3
// 27 R2
// 26 R1
// 25 R0

// Saved conditionally if EXC_RETURN, bit 4 is 0 (zero).
// "vldmiaeq %[r]!, {s16-s31}"
// 24 S31
// 23 S30
// ...
// 10 S17
//  9 S16

// Saved always by context switch handler.
// "stmdb %[r]!, {r4-r9,sl,fp,lr}"
//  8 EXC_RETURN (R14)
//  7 FP (R11)
//  6 SL (R10)
//  5 R9
//  4 R8
//  3 R7
//  2 R6
//  1 R5
//  0 R4 <-- new SP value

  /*
   <!DOCTYPE feature SYSTEM "gdb-target.dtd">
   <target version="1.0">
   <architecture>arm</architecture>
   <feature name="org.gnu.gdb.arm.m-profile">
   <reg name="r0" bitsize="32" regnum="0" type="uint32" group="general"/>
   <reg name="r1" bitsize="32" regnum="1" type="uint32" group="general"/>
   <reg name="r2" bitsize="32" regnum="2" type="uint32" group="general"/>
   <reg name="r3" bitsize="32" regnum="3" type="uint32" group="general"/>
   <reg name="r4" bitsize="32" regnum="4" type="uint32" group="general"/>
   <reg name="r5" bitsize="32" regnum="5" type="uint32" group="general"/>
   <reg name="r6" bitsize="32" regnum="6" type="uint32" group="general"/>
   <reg name="r7" bitsize="32" regnum="7" type="uint32" group="general"/>
   <reg name="r8" bitsize="32" regnum="8" type="uint32" group="general"/>
   <reg name="r9" bitsize="32" regnum="9" type="uint32" group="general"/>
   <reg name="r10" bitsize="32" regnum="10" type="uint32" group="general"/>
   <reg name="r11" bitsize="32" regnum="11" type="uint32" group="general"/>
   <reg name="r12" bitsize="32" regnum="12" type="uint32" group="general"/>
   <reg name="sp" bitsize="32" regnum="13" type="data_ptr" group="general"/>
   <reg name="lr" bitsize="32" regnum="14" type="uint32" group="general"/>
   <reg name="pc" bitsize="32" regnum="15" type="code_ptr" group="general"/>
   <reg name="xpsr" bitsize="32" regnum="25" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-system">
   <reg name="msp" bitsize="32" regnum="26" type="uint32" group="general"/>
   <reg name="psp" bitsize="32" regnum="27" type="uint32" group="general"/>
   <reg name="primask" bitsize="32" regnum="28" type="uint32" group="general"/>
   <reg name="basepri" bitsize="32" regnum="29" type="uint32" group="general"/>
   <reg name="faultmask" bitsize="32" regnum="30" type="uint32" group="general"/>
   <reg name="control" bitsize="32" regnum="31" type="uint32" group="general"/>
   </feature>
   <feature name="org.gnu.gdb.arm.m-float">
   <reg name="fpscr" bitsize="32" regnum="32" type="uint32" group="float"/>
   <reg name="s0" bitsize="32" regnum="33" type="float" group="float"/>
   <reg name="s1" bitsize="32" regnum="34" type="float" group="float"/>
   <reg name="s2" bitsize="32" regnum="35" type="float" group="float"/>
   <reg name="s3" bitsize="32" regnum="36" type="float" group="float"/>
   <reg name="s4" bitsize="32" regnum="37" type="float" group="float"/>
   <reg name="s5" bitsize="32" regnum="38" type="float" group="float"/>
   <reg name="s6" bitsize="32" regnum="39" type="float" group="float"/>
   <reg name="s7" bitsize="32" regnum="40" type="float" group="float"/>
   <reg name="s8" bitsize="32" regnum="41" type="float" group="float"/>
   <reg name="s9" bitsize="32" regnum="42" type="float" group="float"/>
   <reg name="s10" bitsize="32" regnum="43" type="float" group="float"/>
   <reg name="s11" bitsize="32" regnum="44" type="float" group="float"/>
   <reg name="s12" bitsize="32" regnum="45" type="float" group="float"/>
   <reg name="s13" bitsize="32" regnum="46" type="float" group="float"/>
   <reg name="s14" bitsize="32" regnum="47" type="float" group="float"/>
   <reg name="s15" bitsize="32" regnum="48" type="float" group="float"/>
   <reg name="s16" bitsize="32" regnum="49" type="float" group="float"/>
   <reg name="s17" bitsize="32" regnum="50" type="float" group="float"/>
   <reg name="s18" bitsize="32" regnum="51" type="float" group="float"/>
   <reg name="s19" bitsize="32" regnum="52" type="float" group="float"/>
   <reg name="s20" bitsize="32" regnum="53" type="float" group="float"/>
   <reg name="s21" bitsize="32" regnum="54" type="float" group="float"/>
   <reg name="s22" bitsize="32" regnum="55" type="float" group="float"/>
   <reg name="s23" bitsize="32" regnum="56" type="float" group="float"/>
   <reg name="s24" bitsize="32" regnum="57" type="float" group="float"/>
   <reg name="s25" bitsize="32" regnum="58" type="float" group="float"/>
   <reg name="s26" bitsize="32" regnum="59" type="float" group="float"/>
   <reg name="s27" bitsize="32" regnum="60" type="float" group="float"/>
   <reg name="s28" bitsize="32" regnum="61" type="float" group="float"/>
   <reg name="s29" bitsize="32" regnum="62" type="float" group="float"/>
   <reg name="s30" bitsize="32" regnum="63" type="float" group="float"/>
   <reg name="s31" bitsize="32" regnum="64" type="float" group="float"/>
   </feature>
   </target>
   */

// This table encodes the offsets in the µOS++ VFP stack frame for the
// registers defined in the previous XML.
// Used conditionally if EXC_RETURN, bit 4 is 0 (zero).
  static const register_offset_t cortex_m4_vfp_stack_offsets[] =
    {
    //
        25,//R0
        26, // R1
        27, // R2
        28, // R3
        0, // R4
        1, // R5
        2, // R6
        3, // R7
        4, // R8
        5, // R9
        6, // R10
        7, // R11
        29, // R12
        -2, // SP
        30, // LR
        31, // PC
        32, // XPSR

        -1, // MSP
        -1, // PSP
        -1, // PRIMASK
        -1, // BASEPRI
        -1, // FAULTMASK
        -1, // CONTROL
        49, // FPSCR
        33, // S0
        34, // S1
        35, // S2
        36, // S3
        37, // S4
        38, // S5
        39, // S6
        40, // S7
        41, // S8
        42, // S9
        43, // S10
        44, // S11
        45, // S12
        46, // S13
        47, // S14
        48, // S15
        9, // S16
        10, // S17
        11, // S18
        12, // S19
        13, // S20
        14, // S21
        15, // S22
        16, // S23
        17, // S24
        18, // S25
        19, // S26
        20, // S27
        21, // S28
        22, // S29
        23, // S30
        24, // S31
      };

  static const stack_info_t cortex_m4_vfp_stack_info =
    {
    //
        .in_registers = 16 + 1 + 1 + 32 + 1, // R0-R15 + EXC_RETURN +XPSR + S0-S31 + FPSCR
        .out_registers = 16 + 1, // R0-R15 + xPSR
        .offsets = cortex_m4_vfp_stack_offsets, //
        .offsets_size = sizeof(cortex_m4_vfp_stack_offsets)
            / sizeof(cortex_m4_vfp_stack_offsets[0]),
    //
      };

  rtos_t rtos =
    {
    //
        .stack_info = &cortex_m4_stack_info, //
        .stack_info_vfp = &cortex_m4_vfp_stack_info
    //
      };

  const char* thread_states[6] =
    {
    /**/
    "Undefined", "Ready", "Running", "Suspended", "Terminated", "Destroyed"
    /**/
    };

} /* namespace drtm */

// ---------------------------------------------------------------------------

