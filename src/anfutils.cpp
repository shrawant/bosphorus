/*****************************************************************************
Copyright (C) 2018  Davin Choo, Kian Ming A. Chai, DSO National Laboratories

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
***********************************************/

#include <iostream>
#include <unordered_set>

#include "anfutils.hpp"

using std::cout;
using std::endl;
using std::unordered_set;
using std::vector;
USING_NAMESPACE_PBORI

double sample_and_clone(const ConfigData& config,
                        const vector<BoolePolynomial>& eqs,
                        vector<BoolePolynomial>& equations, double log2size)
{
    const polybori::BoolePolyRing& ring(eqs.front().ring());
    const size_t log2fullsz =
        log2(eqs.size()) +
        2 * log2(ring.nVariables()); // assume quadratic equations only
    if (log2fullsz < log2size) {
        // Small system, so clone the entire system
        equations = eqs;
        return log2fullsz;
    } else {
        // randomly select equations until a limit
        unordered_set<BooleMonomial::hash_type> unique;
        double log2uniquesz = 0;
        size_t sampled = 1, reject = 0;
        double rej_rate = 0;
        do {
            rej_rate = static_cast<double>(reject) / sampled;
            size_t sel =
                std::floor(static_cast<double>(rand()) / RAND_MAX * eqs.size());
            ++sampled;
            if (!unique.empty() && rej_rate < 0.8) {
                // accept with probability of not increasing then number of monomials
                size_t out = 0;
                for (const BooleMonomial& mono : eqs[sel])
                    if (unique.find(mono.hash()) == unique.end())
                        ++out;
                if (static_cast<double>(rand()) / RAND_MAX <
                    static_cast<double>(out) / eqs[sel].length()) {
                    ++reject;
                    continue; // reject and continue with do-while loop
                }
            }
            equations.push_back(eqs[sel]);
            for (const BooleMonomial& mono : equations.back())
                unique.insert(mono.hash());
            log2uniquesz = log2(unique.size());
        } while (log2(equations.size()) + log2uniquesz < log2size);
        if (config.verbosity >= 3)
            cout << "c  Selected " << equations.size() << '[' << unique.size()
                 << "] equations with rejection rate " << rej_rate << endl;

        return log2uniquesz;
    }
}

void subsitute(const BooleMonomial& from_mono, const BoolePolynomial& to_poly,
               BoolePolynomial& poly)
{
    BoolePolynomial quotient = poly / from_mono;

    /*if( quotient.isZero() ) // don't need this check
    return;
  else*/
    if (poly.isSingleton()) // for some reason, we need this special case to prevent segfault in some cases
        poly = quotient * to_poly;
    else if (quotient.isOne()) { // ditto
        poly -= from_mono;
        poly += to_poly;
    } else {
        // construct reminder manually to prevent segfault
        BoolePolynomial reminder(0, poly.ring());
        for (const BooleMonomial& mono : poly) {
            if (!mono.reducibleBy(from_mono)) {
                reminder += mono;
            }
        }
        poly = quotient * to_poly;
        if (!reminder.isZero())
            poly += reminder;
    }
}
