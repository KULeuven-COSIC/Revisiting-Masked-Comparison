/*
 * MIT License
 *
 * Copyright (c) 2021-2022: imec-COSIC KU Leuven, 3001 Leuven, Belgium 
 * Authors: Jan-Pieter D'Anvers <janpieter.danvers@esat.kuleuven.be>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SECMULT_H
#define SECMULT_H

#include <stdint.h>
#include <stddef.h>

#ifdef DEBUG
#include <stdio.h>
#include <assert.h>
#endif

void secMult(uint32_t nshares, uint32_t q, uint32_t out[nshares], uint32_t A[nshares], uint32_t B[nshares]);
uint32_t uncompress(uint32_t x, uint32_t compressto, uint32_t q);
void randomq(uint32_t out[2], uint32_t q);

#endif