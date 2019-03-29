/*

This code is based on the cubic spline interpolation code presented in:
Numerical Recipes in C: The Art of Scientific Computing
by
William H. Press,
Brian P. Flannery,
Saul A. Teukolsky, and
William T. Vetterling .
Copyright 1988 (and 1992 for the 2nd edition)

I am assuming zero-offset arrays instead of the unit-offset arrays
suggested by the authors.  You may style me rebel or conformist
depending on your point of view.

Norman Kuring  31-Mar-1999

Norman Kuring  28-May-2015  pklo and pkhi were not getting reset in
        splint() function.  This is now fixed.
        Thx, Robert Strickland, for pointing this out.

*/

#include <stdio.h>
#include <stdlib.h>
#include "spline.h"

void
spline(
        float  x[],
        float  y[],
        int  n,
        float  y2[]
){

    int i,k;
    float p,qn,sig,un,*u;

    u = (float *)malloc((n-1) * sizeof(float));
    if(u == NULL) {
        exit(EXIT_FAILURE);
    }

    y2[0] = u[0] = 0.0;

    for(i = 1; i < n-1; i++){
        sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
        p = sig*y2[i-1] + 2.0;
        y2[i] = (sig - 1.0)/p;
        u[i] = (y[i+1] - y[i])/(x[i+1] - x[i]) - (y[i] - y[i-1])/(x[i] - x[i-1]);
        u[i] = (6.0*u[i]/(x[i+1] - x[i-1]) - sig*u[i-1])/p;
    }

    qn = un = 0.0;

    y2[n-1] = (un - qn*u[n-2])/(qn*y2[n-2] + 1.0);
    for(k = n-2; k >= 0; k--){
        y2[k] = y2[k]*y2[k+1] + u[k];
    }

    free(u);
}

void
splint(
        float  xa[],
        float  ya[],
        float  y2a[],
        int  n,
        float  x,
        float  *y
){

    int    klo,khi,k;
    float    h,b,a;
    static int  pklo=0,pkhi=1;

    /*
    Based on the assumption that sequential calls to this function are made
    with closely-spaced, steadily-increasing values of x, I first try using
    the same values of klo and khi as were used in the previous invocation.
    If that interval is no longer correct, I do a binary search for the
    correct interval.
    */
    if(xa[pklo] <= x && xa[pkhi] > x){
        klo = pklo;
        khi = pkhi;
    }
    else{
        klo = 0;
        khi = n - 1;
        while(khi - klo > 1){
            k = (khi + klo) >> 1;
            if(xa[k] > x) khi = k;
            else          klo = k;
        }
        pklo = klo;
        pkhi = khi;
    }

    h = xa[khi] - xa[klo];
    if(h == 0){
        exit(EXIT_FAILURE);
    }
    a = (xa[khi] - x)/h;
    b = (x - xa[klo])/h;
    *y = a*ya[klo] + b*ya[khi] +
         ((a*a*a - a)*y2a[klo] + (b*b*b - b)*y2a[khi])*(h*h)/6.0;
}