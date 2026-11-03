
#include "advanced.h"
#include "mathcore.h"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AW 396
#define AH 224
#define AC_BG C_RGB(3,6,12)
#define AC_PANEL C_RGB(7,13,23)
#define AC_PANEL2 C_RGB(10,19,32)
#define AC_LINE C_RGB(24,39,56)
#define AC_ACCENT C_RGB(7,17,24)
#define AC_CYAN C_RGB(0,30,31)
#define AC_WHITE C_RGB(29,30,31)
#define AC_MUTED C_RGB(15,19,23)
#define PI 3.14159265358979323846
#define INPUT_CAP 88
#define DATA_CAP 20

static int akey(void)
{
    key_event_t e;
    do { e=getkey(); } while(e.type!=KEYEV_DOWN && e.type!=KEYEV_HOLD);
    return e.key;
}
static void atxt(int x,int y,int c,const char *s) { dtext(x,y,c,s); }
static void ahead(const char *title)
{
    dclear(AC_BG); drect(0,0,AW-1,27,AC_PANEL2); drect(0,27,AW-1,28,AC_LINE);
    atxt(12,7,AC_CYAN,"RICE"); atxt(64,7,AC_WHITE,"ADVANCED MATHS");
    atxt(290,8,AC_MUTED,"fx-CG50"); atxt(12,35,AC_WHITE,title);
}
static void afoot(const char *left,const char *right)
{
    drect(0,200,AW-1,223,AC_PANEL2); drect(0,200,AW-1,200,AC_LINE);
    atxt(12,207,AC_MUTED,left); atxt(245,207,AC_MUTED,right);
}
static void aresult(const char *title,const char **lines,int count)
{
    int top=0;
    if(count<1) count=1;
    for(;;) {
        ahead(title);
        for(int i=0;i<8 && top+i<count;i++) {
            drect(12,59+i*17,383,74+i*17,AC_PANEL);
            atxt(21,62+i*17,AC_WHITE,lines[top+i]);
        }
        char page[32]; snprintf(page,sizeof page,"%d-%d / %d",top+1,top+((count-top)<8?count-top:8),count);
        afoot("UP / DOWN scroll","EXE / EXIT back"); atxt(326,185,AC_MUTED,page); dupdate();
        int k=akey();
        if(k==KEY_UP && top>0) top--;
        else if(k==KEY_DOWN && top+8<count) top++;
        else if(k==KEY_EXE || k==KEY_EXIT) break;
    }
}
static void amessage(const char *title,const char *result,const char *detail)
{
    const char *lines[2]={result,detail}; aresult(title,lines,2);
}
static int amenu(const char *title,const char **items,int n)
{
    int selected=0,top=0;
    for(;;) {
        ahead(title); atxt(18,56,AC_MUTED,"UP / DOWN TO MOVE - EXE TO OPEN");
        if(selected<top) top=selected;
        if(selected>=top+7) top=selected-6;
        for(int row=0;row<7;row++) {
            int idx=top+row; if(idx>=n) break; int y=73+row*18;
            if(idx==selected) { drect(14,y-3,381,y+13,AC_ACCENT); drect(14,y-3,17,y+13,AC_CYAN); }
            char line[70]; snprintf(line,sizeof line,"%02d   %s",idx+1,items[idx]);
            atxt(28,y,idx==selected?AC_WHITE:AC_MUTED,line);
        }
        char page[24]; snprintf(page,sizeof page,"%d / %d",selected+1,n);
        atxt(341,188,AC_MUTED,page); afoot("UP / DOWN move","EXIT back"); dupdate();
        int k=akey();
        if(k==KEY_UP && selected>0) selected--;
        else if(k==KEY_DOWN && selected<n-1) selected++;
        else if(k==KEY_EXE) return selected;
        else if(k==KEY_EXIT) return -1;
    }
}
static int aread_number(const char *prompt,double *v)
{
    char b[INPUT_CAP]=""; int n=0,decimal=0;
    for(;;) {
        ahead("Number input"); drect(12,62,383,128,AC_PANEL); atxt(24,73,AC_MUTED,prompt);
        drect(24,94,371,119,AC_PANEL2); atxt(34,100,AC_WHITE,n?b:"Enter a number...");
        atxt(24,145,AC_MUTED,"0-9 type   . decimal   +/- sign"); atxt(24,163,AC_MUTED,"DEL erase   EXE accept   EXIT cancel");
        afoot("Numeric keypad","RICE input"); dupdate();
        int k=akey(),digit=keycode_digit(k);
        if(digit>=0 && n<INPUT_CAP-2) { b[n++]=(char)('0'+digit); b[n]='\0'; }
        else if(k==KEY_DOT && !decimal && n<INPUT_CAP-2) { if(n==0)b[n++]='0'; b[n++]='.'; b[n]='\0'; decimal=1; }
        else if((k==KEY_NEG||k==KEY_SUB)&&n<INPUT_CAP-2) {
            if(n==0){b[n++]='-';b[n]='\0';}
            else if(b[0]=='-'){memmove(b,b+1,(size_t)n);n--;}
            else {memmove(b+1,b,(size_t)n+1);b[0]='-';n++;}
        } else if(k==KEY_DEL&&n>0){if(b[n-1]=='.')decimal=0;b[--n]='\0';}
        else if(k==KEY_EXE){if(!n||!strcmp(b,"-")||!strcmp(b,"."))continue;*v=strtod(b,NULL);return isfinite(*v);}
        else if(k==KEY_EXIT)return 0;
    }
}
static int aread_integer(const char *prompt,int *value,int minv,int maxv)
{
    double x;
    if(!aread_number(prompt,&x))return 0;
    if(fabs(x-round(x))>1e-8||x<minv||x>maxv){amessage("Input","Enter a whole number in range","The value is outside the permitted range.");return 0;}
    *value=(int)llround(x); return 1;
}
static int aread_poly(const char *prompt,RicePolynomial *p,char *source,size_t cap)
{
    char err[96]; int n=0;
    if(!source || cap < 2) return 0;
    source[0] = 0;
    for(;;) {
        ahead("Polynomial input"); drect(12,62,383,127,AC_PANEL); atxt(24,72,AC_MUTED,prompt);
        drect(24,94,371,120,AC_PANEL2); atxt(31,101,AC_WHITE,source[0]?source:"Enter expression in x...");
        atxt(20,143,AC_MUTED,"0-9  X=x  + - x /  ^  ( )  ."); atxt(20,162,AC_MUTED,"2x and (x+1)(x+2) are supported");
        atxt(20,181,AC_MUTED,"DEL erase  EXE accept  EXIT cancel"); afoot("Calculator keypad","Polynomial"); dupdate();
        int k=akey(),d=keycode_digit(k); char t[8]="";
        if(d>=0){t[0]=(char)('0'+d);t[1]='\0';}
        else if(k==KEY_DOT)strcpy(t,".");
        else if(k==KEY_ADD)strcpy(t,"+");
        else if(k==KEY_SUB||k==KEY_NEG)strcpy(t,"-");
        else if(k==KEY_MUL)strcpy(t,"*");
        else if(k==KEY_DIV||k==KEY_FRAC)strcpy(t,"/");
        else if(k==KEY_POWER)strcpy(t,"^");
        else if(k==KEY_LEFTP)strcpy(t,"(");
        else if(k==KEY_RIGHTP)strcpy(t,")");
        else if(k==KEY_XOT)strcpy(t,"x");
        else if(k==KEY_DEL&&n>0){source[--n]='\0';}
        else if(k==KEY_EXE&&n>0){
            if(rice_parse_polynomial(source,p,err,sizeof err))return 1;
            amessage("Polynomial input",err,"Use brackets, +, -, multiplication and integer powers from 0 to 8.");
            source[0]='\0';n=0;
        } else if(k==KEY_EXIT)return 0;
        if(t[0]&&n+(int)strlen(t)<(int)cap-1){strcat(source,t);n=(int)strlen(source);}
    }
}
static double median_sorted(const double *v,int n)
{
    if(n%2)return v[n/2];
    return (v[n/2-1]+v[n/2])*0.5;
}
static int cmp_double(const void *a,const void *b)
{
    double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y);
}
static int read_dataset(double *v,int *n)
{
    if(!aread_integer("How many data values? (2 to 20)",n,2,DATA_CAP))return 0;
    for(int i=0;i<*n;i++) {
        char prompt[40]; snprintf(prompt,sizeof prompt,"Data value %d of %d",i+1,*n);
        if(!aread_number(prompt,&v[i]))return 0;
    }
    return 1;
}
static void poly_subtract(const RicePolynomial *a,const RicePolynomial *b,RicePolynomial *out)
{
    memset(out,0,sizeof *out); out->degree=a->degree>b->degree?a->degree:b->degree;
    for(int i=0;i<=out->degree;i++)out->c[i]=a->c[i]-b->c[i];
    while(out->degree>0&&fabs(out->c[out->degree])<1e-10)out->degree--;
}
static double choose_binomial(int n,int k)
{
    double r=1.0; if(k>n-k)k=n-k;
    for(int i=1;i<=k;i++) r=r*(n-k+i)/i;
    return r;
}

/* ---------------- Algebra extensions ---------------- */
static void algebra_factorise(void)
{
    RicePolynomial p; char source[INPUT_CAP]; double lo,hi,roots[8];
    if(!aread_poly("Enter a polynomial to factorise", &p,source,sizeof source)||
       !aread_number("Search interval lower x", &lo)||!aread_number("Search interval upper x",&hi))return;
    if(lo>=hi){amessage("Factorise polynomial","Invalid interval","The lower limit must be below the upper limit.");return;}
    int n=rice_polynomial_real_roots(&p,lo,hi,roots,8);
    char lines[10][80]; const char *out[10]; int count=0;
    snprintf(lines[count],80,"Polynomial: %.65s",source);out[count]=lines[count];count++;
    if(!n){snprintf(lines[count],80,"No real roots found in that interval.");out[count]=lines[count];count++;}
    else {
        snprintf(lines[count],80,"Approximate real linear factors:");out[count]=lines[count];count++;
        for(int i=0;i<n&&count<9;i++){
            snprintf(lines[count],80,"Root %.8g gives factor (x %c %.8g)",roots[i],roots[i]<0?'+':'-',fabs(roots[i]));
            out[count]=lines[count];count++;
        }
        snprintf(lines[count],80,"Leading coefficient: %.8g",p.c[p.degree]);out[count]=lines[count];count++;
        snprintf(lines[count],80,"Numerical factors; exact irrational factors may remain.");out[count]=lines[count];count++;
    }
    aresult("Factorise polynomial",out,count);
}
static void algebra_fraction(void)
{
    RicePolynomial n,d,g,qn,qd,rn,rd; char sn[INPUT_CAP],sd[INPUT_CAP],a[112],b[112];
    if(!aread_poly("Numerator polynomial", &n,sn,sizeof sn)||!aread_poly("Denominator polynomial",&d,sd,sizeof sd))return;
    if(d.degree==0&&fabs(d.c[0])<1e-10){amessage("Algebraic fraction","Denominator is zero","A fraction cannot have a zero denominator.");return;}
    if(!rice_polynomial_gcd(&n,&d,&g)||!rice_polynomial_divide(&n,&g,&qn,&rn)||!rice_polynomial_divide(&d,&g,&qd,&rd)){
        amessage("Algebraic fraction","Could not simplify","Try a polynomial with ordinary numeric coefficients.");return;
    }
    if(rn.degree>0||fabs(rn.c[0])>1e-7||rd.degree>0||fabs(rd.c[0])>1e-7){
        amessage("Algebraic fraction","No reliable common factor","The polynomial division did not produce an exact cancellation.");return;
    }
    rice_polynomial_format(&qn,a,sizeof a);rice_polynomial_format(&qd,b,sizeof b);
    char lines[4][100];const char *out[4];char common[100];
    rice_polynomial_format(&g,common,sizeof common);
    snprintf(lines[0],100,"Original: (%.35s) / (%.35s)",sn,sd);out[0]=lines[0];
    snprintf(lines[1],100,"Common factor: %.82s",common);out[1]=lines[1];
    snprintf(lines[2],100,"Simplified: (%.35s) / (%.35s)",a,b);out[2]=lines[2];
    snprintf(lines[3],100,"Restrictions: original denominator must not be zero.");out[3]=lines[3];
    aresult("Simplify algebraic fraction",out,4);
}
static void algebra_complete_square(void)
{
    double a,b,c; char lines[4][96];const char *out[4];
    if(!aread_number("Coefficient a", &a)||!aread_number("Coefficient b",&b)||!aread_number("Constant c",&c))return;
    if(fabs(a)<1e-12){amessage("Complete the square","a cannot be zero","Enter a genuine quadratic.");return;}
    double h=b/(2*a),k=c-b*b/(4*a);
    snprintf(lines[0],96,"Original: %.8gx^2 %+.8gx %+.8g",a,b,c);out[0]=lines[0];
    snprintf(lines[1],96,"Completed-square form:");out[1]=lines[1];
    snprintf(lines[2],96,"%.8g(x %c %.8g)^2 %+.8g",a,h<0?'-':'+',fabs(h),k);out[2]=lines[2];
    snprintf(lines[3],96,"Vertex: (%.8g, %.8g)",-h,k);out[3]=lines[3];
    aresult("Complete the square",out,4);
}
static void algebra_formula(void)
{
    static const char *items[]={"y = mx + c  (solve x)","v = u + at  (solve t)","F = ma  (solve a)","Speed = distance / time","Density = mass / volume  (solve V)","Pressure = force / area  (solve A)","Triangle area = base x height / 2","Circumference = 2 pi r  (solve r)"};
    int c=amenu("Rearrange a formula",items,8);double x,y,z,r;char result[96],detail[96];
    if(c<0)return;
    switch(c){
    case 0:
        if(!aread_number("Known y",&x)||!aread_number("Gradient m",&y)||!aread_number("Intercept c",&z))return;
        if(fabs(y)<1e-12){amessage("Rearrange formula","m cannot be zero","x cannot be found when the gradient is zero.");return;}
        r=(x-z)/y;snprintf(detail,sizeof detail,"x = (y - c) / m");break;
    case 1:
        if(!aread_number("Final velocity v",&x)||!aread_number("Initial velocity u",&y)||!aread_number("Acceleration a",&z))return;
        if(fabs(z)<1e-12){amessage("Rearrange formula","a cannot be zero","Time cannot be found by dividing by zero.");return;}
        r=(x-y)/z;snprintf(detail,sizeof detail,"t = (v - u) / a");break;
    case 2:
        if(!aread_number("Force F",&x)||!aread_number("Mass m",&y))return;
        if(fabs(y)<1e-12){amessage("Rearrange formula","Mass cannot be zero","Acceleration = force / mass.");return;}
        r=x/y;snprintf(detail,sizeof detail,"a = F / m");break;
    case 3:
        {int q=amenu("Speed formula", (const char*[]){"Find distance d = speed x time","Find time t = distance / speed"},2);if(q<0)return;
        if(!aread_number(q==0?"Speed":"Distance",&x)||!aread_number(q==0?"Time":"Speed",&y))return;
        if(q==0){r=x*y;snprintf(detail,sizeof detail,"d = speed x time");}
        else {if(fabs(y)<1e-12){amessage("Rearrange formula","Speed cannot be zero","Time cannot be found by division by zero.");return;}r=x/y;snprintf(detail,sizeof detail,"t = distance / speed");}break;}
    case 4:
        if(!aread_number("Mass m",&x)||!aread_number("Density",&y))return;
        if(fabs(y)<1e-12){amessage("Rearrange formula","Density cannot be zero","Volume cannot be found by division by zero.");return;}
        r=x/y;snprintf(detail,sizeof detail,"V = mass / density");break;
    case 5:
        if(!aread_number("Force F",&x)||!aread_number("Pressure",&y))return;
        if(fabs(y)<1e-12){amessage("Rearrange formula","Pressure cannot be zero","Area cannot be found by division by zero.");return;}
        r=x/y;snprintf(detail,sizeof detail,"A = force / pressure");break;
    case 6:
        if(!aread_number("Triangle area",&x)||!aread_number("Base",&y))return;
        if(fabs(y)<1e-12){amessage("Rearrange formula","Base cannot be zero","Height cannot be found by division by zero.");return;}
        r=2*x/y;snprintf(detail,sizeof detail,"height = 2 x area / base");break;
    default:
        if(!aread_number("Circumference",&x))return;
        r=x/(2*PI);snprintf(detail,sizeof detail,"r = circumference / (2 pi)");break;
    }
    snprintf(result,sizeof result,"Answer = %.10g",r);amessage("Rearranged formula",result,detail);
}
static void advanced_algebra(int choice)
{
    switch(choice){case 0:algebra_factorise();break;case 1:algebra_fraction();break;case 2:algebra_formula();break;case 3:algebra_complete_square();break;}
}

/* ---------------- Equations and graph analysis ---------------- */
static int get_interval(double *lo,double *hi)
{
    if(!aread_number("Search interval lower x",lo)||!aread_number("Search interval upper x",hi))return 0;
    if(*lo>=*hi){amessage("Interval","Lower limit must be smaller","Choose an interval with lower x below upper x.");return 0;}return 1;
}
static void show_roots(const char *title,const RicePolynomial *p,double lo,double hi)
{
    double roots[8];int n=rice_polynomial_real_roots(p,lo,hi,roots,8);char lines[10][80];const char *out[10];int c=0;
    if(!n){out[c++]="No real roots found in the selected interval.";}
    else {for(int i=0;i<n&&c<9;i++){snprintf(lines[c],80,"Root %d: x = %.10g",i+1,roots[i]);out[c]=lines[c];c++;}}
    aresult(title,out,c);
}
static void graph_roots(void)
{
    RicePolynomial p;char src[INPUT_CAP];double lo,hi;
    if(!aread_poly("Enter polynomial f(x) = 0",&p,src,sizeof src)||!get_interval(&lo,&hi))return;
    show_roots("Polynomial roots",&p,lo,hi);
}
static void graph_intersections(void)
{
    RicePolynomial a,b,d;char sa[INPUT_CAP],sb[INPUT_CAP];double lo,hi,roots[8];
    if(!aread_poly("First function f(x)",&a,sa,sizeof sa)||!aread_poly("Second function g(x)",&b,sb,sizeof sb)||!get_interval(&lo,&hi))return;
    poly_subtract(&a,&b,&d);int n=rice_polynomial_real_roots(&d,lo,hi,roots,8);
    char lines[10][88];const char *out[10];int c=0;
    if(!n)out[c++]="No intersections found in that interval.";
    else for(int i=0;i<n&&c<9;i++){double y=rice_polynomial_evaluate(&a,roots[i]);snprintf(lines[c],88,"P%d: (%.8g, %.8g)",i+1,roots[i],y);out[c]=lines[c];c++;}
    aresult("Function intersections",out,c);
}
static void graph_turning(void)
{
    RicePolynomial p,d,dd;char src[INPUT_CAP];double lo,hi,roots[8];
    if(!aread_poly("Enter polynomial f(x)",&p,src,sizeof src)||!get_interval(&lo,&hi))return;
    rice_polynomial_derivative(&p,&d);rice_polynomial_derivative(&d,&dd);
    int n=rice_polynomial_real_roots(&d,lo,hi,roots,8);char lines[10][88];const char *out[10];int c=0;
    if(!n)out[c++]="No stationary points found in that interval.";
    else for(int i=0;i<n&&c<9;i++){double y=rice_polynomial_evaluate(&p,roots[i]),second=rice_polynomial_evaluate(&dd,roots[i]);const char *kind=second>1e-7?"local minimum":second< -1e-7?"local maximum":"stationary / possible inflection";snprintf(lines[c],88,"x=%.8g  y=%.8g  %s",roots[i],y,kind);out[c]=lines[c];c++;}
    aresult("Turning points",&out[0],c);
}
static void graph_table(void)
{
    RicePolynomial p;char src[INPUT_CAP];double start,end,step;
    if(!aread_poly("Enter polynomial f(x)",&p,src,sizeof src)||!aread_number("Start x",&start)||!aread_number("End x",&end)||!aread_number("Step size",&step))return;
    if(step==0||((end-start)*step<0)){amessage("Table of values","Invalid step","Step must move from the start toward the end.");return;}
    int n=(int)floor((end-start)/step+1e-9)+1;if(n<1||n>60){amessage("Table of values","Too many rows","Choose a range containing between 1 and 60 rows.");return;}
    int top=0;
    for(;;){ahead("Table of values");atxt(18,56,AC_CYAN,"x                       f(x)");
        for(int i=0;i<8&&top+i<n;i++){char line[72];double x=start+(top+i)*step,y=rice_polynomial_evaluate(&p,x);snprintf(line,sizeof line,"% .8g                 % .9g",x,y);atxt(20,78+i*15,AC_WHITE,line);}
        char page[30];snprintf(page,sizeof page,"%d-%d / %d",top+1,top+((n-top)<8?n-top:8),n);atxt(326,185,AC_MUTED,page);
        afoot("UP / DOWN scroll","EXIT back");dupdate();int k=akey();
        if(k==KEY_UP&&top>0)top--;else if(k==KEY_DOWN&&top+8<n)top++;else if(k==KEY_EXIT||k==KEY_EXE)break;
    }
}
static void graph_transform(void)
{
    RicePolynomial p,t;char src[INPUT_CAP],formatted[120];double a,b,h,k;
    if(!aread_poly("Enter original polynomial f(x)",&p,src,sizeof src)||!aread_number("Vertical scale a",&a)||!aread_number("Horizontal scale b",&b)||!aread_number("Horizontal shift h",&h)||!aread_number("Vertical shift k",&k))return;
    if(fabs(b)<1e-12){amessage("Transform function","Horizontal scale b cannot be zero","A zero horizontal scale collapses the input to a constant.");return;}
    memset(&t,0,sizeof t);t.degree=p.degree;
    for(int i=0;i<=p.degree;i++)for(int j=0;j<=i;j++)
        t.c[j]+=a*p.c[i]*pow(b,(double)i)*choose_binomial(i,j)*pow(-h,(double)(i-j));
    t.c[0]+=k;
    for(int i=0;i<=t.degree;i++)if(!isfinite(t.c[i])){amessage("Transform function","Coefficient overflow","Choose smaller coefficients or transformation values.");return;}
    if(!rice_polynomial_format(&t,formatted,sizeof formatted)){amessage("Transform function","Result too long","The transformed expression could not fit on screen.");return;}
    char lines[5][100];const char *out[5];
    snprintf(lines[0],100,"Rule: y = a f(b(x - h)) + k");out[0]=lines[0];
    snprintf(lines[1],100,"a=%.6g  b=%.6g  h=%.6g  k=%.6g",a,b,h,k);out[1]=lines[1];
    snprintf(lines[2],100,"Original: %s",src);out[2]=lines[2];
    snprintf(lines[3],100,"Transformed: y = %.80s",formatted);out[3]=lines[3];
    snprintf(lines[4],100,"This expands the transformed polynomial; b < 0 includes reflection.");out[4]=lines[4];
    aresult("Function transformations",out,5);
}
static void graph_plot(void)
{
    RicePolynomial p;char src[INPUT_CAP];double lo,hi;
    if(!aread_poly("Enter polynomial f(x)",&p,src,sizeof src)||!get_interval(&lo,&hi))return;
    double ymin=INFINITY,ymax=-INFINITY;
    for(int i=0;i<=100;i++){double x=lo+(hi-lo)*i/100.0,y=rice_polynomial_evaluate(&p,x);if(isfinite(y)){if(y<ymin)ymin=y;if(y>ymax)ymax=y;}}
    if(!isfinite(ymin)||!isfinite(ymax)){amessage("Graph plot","Cannot plot these values","The selected interval produced values outside the numeric range.");return;}
    if(fabs(ymax-ymin)<1e-12){ymin-=1;ymax+=1;}else{double pad=(ymax-ymin)*0.08;ymin-=pad;ymax+=pad;}
    ahead("Polynomial plot");
    int x0=28,y0=60,w=340,h=120;
    if(lo<=0&&hi>=0){int ax=x0+(int)((0-lo)/(hi-lo)*w);drect(ax,y0,ax+1,y0+h,AC_LINE);}
    if(ymin<=0&&ymax>=0){int ay=y0+(int)((ymax)/(ymax-ymin)*h);drect(x0,ay,x0+w,ay+1,AC_LINE);}
    drect(x0,y0,x0+w,y0,AC_LINE);drect(x0,y0+h,x0+w,y0+h,AC_LINE);
    int lastx=-1,lasty=-1;
    for(int i=0;i<=w;i++){double x=lo+(hi-lo)*i/w,y=rice_polynomial_evaluate(&p,x);
        if(isfinite(y)&&y>=ymin&&y<=ymax){int py=y0+h-(int)((y-ymin)/(ymax-ymin)*h);
            if(lastx>=0&&abs(py-lasty)<=5){int mn=py<lasty?py:lasty,mx=py>lasty?py:lasty;drect(x0+i-1,mn,x0+i,mx,AC_CYAN);}else drect(x0+i,py,x0+i,py,AC_CYAN);lastx=i;lasty=py;}else lastx=-1;}
    char label[60];snprintf(label,sizeof label,"x: %.5g to %.5g",lo,hi);atxt(18,185,AC_WHITE,label);
    afoot("EXE / EXIT back","RICE graph");dupdate();int key;do { key=akey(); } while(key!=KEY_EXE && key!=KEY_EXIT);
}
static void equations_graphs(int choice)
{
    switch(choice){case 0:graph_roots();break;case 1:graph_intersections();break;case 2:graph_turning();break;case 3:graph_table();break;case 4:graph_transform();break;case 5:graph_plot();break;}
}

/* ---------------- Statistics ---------------- */
static void stats_summary(void)
{
    double v[DATA_CAP],s[DATA_CAP];int n;if(!read_dataset(v,&n))return;
    memcpy(s,v,(size_t)n*sizeof(double));qsort(s,(size_t)n,sizeof(double),cmp_double);
    double sum=0,sq=0;for(int i=0;i<n;i++) sum+=s[i];
    double mean=sum/n; for(int i=0;i<n;i++) sq+=(s[i]-mean)*(s[i]-mean);
    double q1=median_sorted(s,n/2),q3=median_sorted(s+(n+1)/2,n/2);
    int maxfreq=1;double mode=s[0];int modes=0;
    for(int i=0;i<n;){int j=i+1;while(j<n&&fabs(s[j]-s[i])<1e-9)j++;int f=j-i;if(f>maxfreq){maxfreq=f;mode=s[i];modes=1;}else if(f==maxfreq&&f>1)modes++;i=j;}
    char lines[9][88];const char *out[9];
    snprintf(lines[0],88,"Count: %d    Mean: %.9g",n,sum/n);out[0]=lines[0];
    snprintf(lines[1],88,"Median: %.9g    Range: %.9g",median_sorted(s,n),s[n-1]-s[0]);out[1]=lines[1];
    snprintf(lines[2],88,"Minimum: %.9g    Maximum: %.9g",s[0],s[n-1]);out[2]=lines[2];
    snprintf(lines[3],88,"Lower quartile Q1: %.9g",q1);out[3]=lines[3];
    snprintf(lines[4],88,"Upper quartile Q3: %.9g",q3);out[4]=lines[4];
    snprintf(lines[5],88,"Interquartile range: %.9g",q3-q1);out[5]=lines[5];
    snprintf(lines[6],88,"Population SD: %.9g",sqrt(sq/n));out[6]=lines[6];
    snprintf(lines[7],88,"Sample SD: %.9g",n>1?sqrt(sq/(n-1)):0);out[7]=lines[7];
    if(maxfreq<=1)snprintf(lines[8],88,"Mode: no repeated value");else snprintf(lines[8],88,modes>1?"One of multiple modes: %.9g (freq %d)":"Mode: %.9g (freq %d)",mode,maxfreq);
    out[8]=lines[8];aresult("Statistical summary",out,9);
}
static int unique_values(const double *sorted,int n,double *values,int *freq)
{
    int m=0;for(int i=0;i<n;){int j=i+1;while(j<n&&fabs(sorted[j]-sorted[i])<1e-9)j++;values[m]=sorted[i];freq[m]=j-i;m++;i=j;}return m;
}
static void stats_frequency(int cumulative)
{
    double v[DATA_CAP],s[DATA_CAP],values[DATA_CAP];int f[DATA_CAP],n;if(!read_dataset(v,&n))return;
    memcpy(s,v,(size_t)n*sizeof(double));qsort(s,(size_t)n,sizeof(double),cmp_double);int m=unique_values(s,n,values,f),cum=0;
    char lines[DATA_CAP+2][80];const char *out[DATA_CAP+2];snprintf(lines[0],80,cumulative?"Value       Frequency    Cumulative":"Value       Frequency");out[0]=lines[0];
    for(int i=0;i<m;i++){cum+=f[i];if(cumulative)snprintf(lines[i+1],80,"% .8g       %d              %d",values[i],f[i],cum);else snprintf(lines[i+1],80,"% .8g       %d",values[i],f[i]);out[i+1]=lines[i+1];}
    aresult(cumulative?"Cumulative frequency":"Frequency table",out,m+1);
}
static void stats_histogram(void)
{
    double v[DATA_CAP],mn=INFINITY,mx=-INFINITY;int n,bins;if(!read_dataset(v,&n)||!aread_integer("Number of classes (2 to 8)",&bins,2,8))return;
    for(int i=0;i<n;i++){if(v[i]<mn)mn=v[i];if(v[i]>mx)mx=v[i];}
    double width=(mx-mn)/bins;if(width<=1e-12)width=1.0;
    int f[8]={0};for(int i=0;i<n;i++){int b=(int)((v[i]-mn)/width);if(b>=bins)b=bins-1;if(b<0)b=0;f[b]++;}
    char lines[9][80];const char *out[9];
    for(int b=0;b<bins;b++){char bar[22]="";int k=f[b];if(k>18)k=18;for(int j=0;j<k;j++)bar[j]='#';bar[k]='\0';snprintf(lines[b],80,"%.5g-%.5g  %2d  %s",mn+b*width,mn+(b+1)*width,f[b],bar);out[b]=lines[b];}
    aresult("Histogram (class counts)",out,bins);
    ahead("Histogram");
    int chart_x=32,chart_y=62,chart_w=332,chart_h=112,maxf=1;
    for(int b=0;b<bins;b++)if(f[b]>maxf)maxf=f[b];
    drect(chart_x,chart_y+chart_h,chart_x+chart_w,chart_y+chart_h+1,AC_LINE);
    int gap=5,bw=(chart_w-gap*(bins-1))/bins;
    for(int b=0;b<bins;b++){
        int bh=(int)((double)f[b]/maxf*(chart_h-8));if(bh<1&&f[b]>0)bh=1;
        int x=chart_x+b*(bw+gap);drect(x,chart_y+chart_h-bh,x+bw-1,chart_y+chart_h-1,AC_CYAN);
        char lab[18];snprintf(lab,sizeof lab,"%d",f[b]);atxt(x,chart_y+chart_h+6,AC_WHITE,lab);
    }
    atxt(18,184,AC_MUTED,"Bar heights show frequency in each class.");
    afoot("EXE / EXIT back","Histogram");dupdate();int key;do{key=akey();}while(key!=KEY_EXE&&key!=KEY_EXIT);
}
static void stats_boxplot(void)
{
    double v[DATA_CAP],s[DATA_CAP];int n;if(!read_dataset(v,&n))return;memcpy(s,v,(size_t)n*sizeof(double));qsort(s,(size_t)n,sizeof(double),cmp_double);
    double q1=median_sorted(s,n/2),med=median_sorted(s,n),q3=median_sorted(s+(n+1)/2,n/2);
    char lines[6][80];const char *out[6];
    snprintf(lines[0],80,"Minimum: %.9g",s[0]);out[0]=lines[0];
    snprintf(lines[1],80,"Lower quartile: %.9g",q1);out[1]=lines[1];
    snprintf(lines[2],80,"Median: %.9g",med);out[2]=lines[2];
    snprintf(lines[3],80,"Upper quartile: %.9g",q3);out[3]=lines[3];
    snprintf(lines[4],80,"Maximum: %.9g",s[n-1]);out[4]=lines[4];
    snprintf(lines[5],80,"Five-number summary for a box plot.");out[5]=lines[5];aresult("Box plot data",out,6);
    double range=s[n-1]-s[0];if(range<1e-12)range=1.0;
    int left=30,width=336,y=115;
    int px0=left,px1=left+(int)((s[n-1]-s[0])/range*width);
    int pq1=left+(int)((q1-s[0])/range*width),pm=left+(int)((median_sorted(s,n)-s[0])/range*width),pq3=left+(int)((q3-s[0])/range*width);
    ahead("Box-and-whisker plot");
    drect(px0,y-1,pq1,y+1,AC_WHITE);drect(pq3,y-1,px1,y+1,AC_WHITE);
    drect(pq1,y-17,pq3,y+17,AC_PANEL);drect(pq1,y-17,pq3,y-16,AC_CYAN);drect(pq1,y+16,pq3,y+17,AC_CYAN);
    drect(pq1,y-17,pq1+1,y+17,AC_CYAN);drect(pq3-1,y-17,pq3,y+17,AC_CYAN);
    drect(pm,y-18,pm+1,y+18,AC_WHITE);drect(px0,y-8,px0+1,y+8,AC_WHITE);drect(px1-1,y-8,px1,y+8,AC_WHITE);
    char lo[32],hi[32];snprintf(lo,sizeof lo,"%.6g",s[0]);snprintf(hi,sizeof hi,"%.6g",s[n-1]);
    atxt(20,154,AC_WHITE,lo);atxt(332,154,AC_WHITE,hi);atxt(20,180,AC_MUTED,"Whiskers: min/max; box: Q1 to Q3; line: median.");
    afoot("EXE / EXIT back","Box plot");dupdate();int key;do{key=akey();}while(key!=KEY_EXE&&key!=KEY_EXIT);
}
static void stats_regression(void)
{
    double x[DATA_CAP],y[DATA_CAP];int n;if(!aread_integer("Number of coordinate pairs (2 to 20)",&n,2,DATA_CAP))return;
    for(int i=0;i<n;i++){char p[40];snprintf(p,sizeof p,"Pair %d: x value",i+1);if(!aread_number(p,&x[i]))return;snprintf(p,sizeof p,"Pair %d: y value",i+1);if(!aread_number(p,&y[i]))return;}
    double sx=0,sy=0,sxx=0,syy=0,sxy=0;for(int i=0;i<n;i++){sx+=x[i];sy+=y[i];sxx+=x[i]*x[i];syy+=y[i]*y[i];sxy+=x[i]*y[i];}
    double den=n*sxx-sx*sx;if(fabs(den)<1e-12){amessage("Regression","No unique regression line","All x values are equal.");return;}
    double m=(n*sxy-sx*sy)/den,c=(sy-m*sx)/n,denr=sqrt(fmax(0,(n*sxx-sx*sx)*(n*syy-sy*sy)));
    double r=denr>1e-12?(n*sxy-sx*sy)/denr:NAN;
    char lines[4][88];const char *out[4];
    snprintf(lines[0],88,"Regression line: y = %.9g x %+.9g",m,c);out[0]=lines[0];
    snprintf(lines[1],88,"Gradient: %.9g    Intercept: %.9g",m,c);out[1]=lines[1];
    snprintf(lines[2],88,"Correlation coefficient r = %.9g",r);out[2]=lines[2];
    snprintf(lines[3],88,"Pearson linear regression / correlation.");out[3]=lines[3];aresult("Regression and correlation",out,4);
}
static void stats_menu(int choice)
{
    switch(choice){case 0:stats_summary();break;case 1:stats_frequency(0);break;case 2:stats_histogram();break;case 3:stats_frequency(1);break;case 4:stats_boxplot();break;case 5:stats_regression();break;}
}

/* ---------------- Geometry ---------------- */
static void geo_bearings(void)
{
    static const char *items[]={"Angle between two bearings","Bearing from east/north displacement","Third angle in a triangle"};
    int c=amenu("Angles and bearings",items,3);double a,b,r;char line[88];
    if(c<0)return;
    if(c==0){if(!aread_number("First bearing (degrees)",&a)||!aread_number("Second bearing (degrees)",&b))return;r=fabs(fmod(b-a,360.0));if(r>180)r=360-r;snprintf(line,sizeof line,"Smaller angle = %.9g degrees",r);}
    else if(c==1){if(!aread_number("East displacement (positive east)",&a)||!aread_number("North displacement (positive north)",&b))return;if(fabs(a)+fabs(b)<1e-12){amessage("Bearing","Direction is undefined","The displacement cannot be zero.");return;}r=atan2(a,b)*180/PI;if(r<0)r+=360;if(r>=359.5) r=0;
        snprintf(line,sizeof line,"Three-figure bearing = %03.0f degrees",r); }
    else {if(!aread_number("First angle",&a)||!aread_number("Second angle",&b))return;r=180-a-b;if(a<=0||b<=0||r<=0){amessage("Triangle angles","Invalid angles","The three interior angles must be positive and sum to 180 degrees.");return;}snprintf(line,sizeof line,"Third angle = %.9g degrees",r);}
    amessage("Angle / bearing result",line,"Bearings are measured clockwise from north.");
}
static void geo_pythagoras(void)
{
    static const char *items[]={"Find the hypotenuse","Find a shorter side"};int c=amenu("Pythagoras",items,2);double a,b,r;
    if(c<0) return;
    if(!aread_number(c==0?"First shorter side":"Hypotenuse",&a)||!aread_number(c==0?"Second shorter side":"Other shorter side",&b)) return;
    if(a<=0||b<=0||(c==1&&b>=a)){amessage("Pythagoras","Invalid side lengths","All sides must be positive and a hypotenuse must be the longest side.");return;}
    r=c==0?sqrt(a*a+b*b):sqrt(a*a-b*b);char out[64];snprintf(out,sizeof out,"Missing side = %.10g",r);amessage("Pythagoras result",out,"a^2 + b^2 = c^2");
}
static void geo_trig(void)
{
    static const char *side[]={"Opposite","Adjacent","Hypotenuse"};int known,wanted;double angle,value; 
    if(!aread_number("Angle in degrees (0 to 90)",&angle)||!aread_number("Known side length",&value))return;
    if(angle<=0||angle>=90||value<=0){amessage("Trigonometry","Invalid input","Use an acute angle and a positive side length.");return;}
    known=amenu("Which side do you know?",side,3);if(known<0)return;wanted=amenu("Which side do you need?",side,3);if(wanted<0)return;if(known==wanted){amessage("Trigonometry","Choose a different side","The known and unknown sides must be different.");return;}
    double co[3]={sin(angle*PI/180),cos(angle*PI/180),1.0};
    double result=value*co[wanted]/co[known];char line[80];snprintf(line,sizeof line,"%s = %.10g",side[wanted],result);
    amessage("SOHCAHTOA result",line,"For an acute right-angled triangle, opposite/hypotenuse = sin(angle), adjacent/hypotenuse = cos(angle).");
}
static void geo_area_volume(void)
{
    static const char *items[]={"Rectangle area","Triangle area","Trapezium area","Circle area","Parallelogram area","Cuboid volume","Cylinder volume","Sphere volume","Cone volume"};
    int c=amenu("Area and volume",items,9);double a=1,b=1,d=1,r;char out[88];
    if(c<0)return;
    switch(c){
    case 0:if(!aread_number("Length",&a)||!aread_number("Width",&b))return;r=a*b;break;
    case 1:if(!aread_number("Base",&a)||!aread_number("Perpendicular height",&b))return;r=a*b/2;break;
    case 2:if(!aread_number("Parallel side a",&a)||!aread_number("Parallel side b",&b)||!aread_number("Height",&d))return;r=(a+b)*d/2;break;
    case 3:if(!aread_number("Radius",&a))return;r=PI*a*a;break;
    case 4:if(!aread_number("Base",&a)||!aread_number("Perpendicular height",&b))return;r=a*b;break;
    case 5:if(!aread_number("Length",&a)||!aread_number("Width",&b)||!aread_number("Height",&d))return;r=a*b*d;break;
    case 6:if(!aread_number("Radius",&a)||!aread_number("Height",&b))return;r=PI*a*a*b;break;
    case 7:if(!aread_number("Radius",&a))return;r=4.0*PI*a*a*a/3.0;break;
    default:if(!aread_number("Radius",&a)||!aread_number("Height",&b))return;r=PI*a*a*b/3.0;break;
    }
    if(a<=0||b<=0||d<=0){amessage("Area and volume","Dimensions must be positive","Enter positive lengths, radii and heights.");return;}
    if(!isfinite(r)){amessage("Area and volume","Result outside range","Try smaller dimensions.");return;}
    snprintf(out,sizeof out,"Result = %.10g",r);amessage("Area and volume",out,"Check that all dimensions use consistent units.");
}
static void geo_similarity(void)
{
    static const char *items[]={"Find a corresponding length","Find corresponding area","Find corresponding volume","Check triangle congruence (SSS)"};
    int c=amenu("Similarity and congruence",items,4);double a,k,r;char out[88];
    if(c<0)return;
    if(c<3){
        if(!aread_number("Original measurement",&a)||!aread_number("Scale factor k",&k))return;
        if(k<=0){amessage("Similarity","Scale factor must be positive","Use a positive scale factor.");return;}
        if(c==0)r=a*k;else if(c==1)r=a*k*k;else r=a*k*k*k;
        snprintf(out,sizeof out,"New measurement = %.10g",r);
        amessage("Similarity result",out,c==0?"Length factor: k":c==1?"Area factor: k squared":"Volume factor: k cubed");
    }else{
        double x[3],y[3];for(int i=0;i<3;i++){char p[48];snprintf(p,sizeof p,"Triangle A side %d",i+1);if(!aread_number(p,&x[i]))return;}
        for(int i=0;i<3;i++){char p[48];snprintf(p,sizeof p,"Triangle B side %d",i+1);if(!aread_number(p,&y[i]))return;}
        qsort(x,3,sizeof(double),cmp_double);qsort(y,3,sizeof(double),cmp_double);
        int valid=x[0]>0&&y[0]>0&&x[0]+x[1]>x[2]&&y[0]+y[1]>y[2];
        int same=valid;for(int i=0;i<3;i++)if(fabs(x[i]-y[i])>1e-8*(1+fabs(x[i])))same=0;
        amessage("SSS congruence",!valid?"Invalid triangle side lengths":same?"Congruent by SSS":"Not congruent by SSS","Both sets must form valid triangles and all corresponding sides must match.");
    }
}
static void geo_coordinates(void)
{
    double x1,y1,x2,y2;if(!aread_number("Point A x",&x1)||!aread_number("Point A y",&y1)||!aread_number("Point B x",&x2)||!aread_number("Point B y",&y2))return;
    double dx=x2-x1,dy=y2-y1,dist=hypot(dx,dy),m;
    char lines[5][88];const char *out[5];
    snprintf(lines[0],88,"Distance AB = %.10g",dist);out[0]=lines[0];
    snprintf(lines[1],88,"Midpoint = (%.8g, %.8g)",(x1+x2)/2,(y1+y2)/2);out[1]=lines[1];
    if(fabs(dx)<1e-12)snprintf(lines[2],88,"Gradient: undefined (vertical line)");else {m=dy/dx;snprintf(lines[2],88,"Gradient = %.10g",m);}out[2]=lines[2];
    snprintf(lines[3],88,"Change in x = %.8g; change in y = %.8g",dx,dy);out[3]=lines[3];
    snprintf(lines[4],88,"Line equation: y = mx + c (when not vertical).");out[4]=lines[4];aresult("Coordinate geometry",out,5);
}
static void geo_vectors(void)
{
    static const char *items[]={"Add two vectors","Subtract vectors A - B","Magnitude of a vector","Dot product","Multiply vector by scalar"};
    int c=amenu("Vector calculations",items,5);double ax,ay,bx,by,k,r;char lines[3][88];const char *out[3];
    if(c<0) return;
    if(!aread_number("Vector A x",&ax)||!aread_number("Vector A y",&ay)) return;
    if(c==2){r=hypot(ax,ay);snprintf(lines[0],88,"|A| = %.10g",r);out[0]=lines[0];aresult("Vector magnitude",out,1);return;}
    if(c==4){if(!aread_number("Scalar k",&k))return;snprintf(lines[0],88,"kA = (%.9g, %.9g)",k*ax,k*ay);out[0]=lines[0];aresult("Scalar multiplication",out,1);return;}
    if(!aread_number("Vector B x",&bx)||!aread_number("Vector B y",&by))return;
    if(c==3)r=ax*bx+ay*by;else r=0;
    if(c==3){snprintf(lines[0],88,"A dot B = %.10g",r);out[0]=lines[0];aresult("Dot product",out,1);}
    else {double sign=c==0?1.0:-1.0;snprintf(lines[0],88,"Result = (%.9g, %.9g)",ax+sign*bx,ay+sign*by);out[0]=lines[0];snprintf(lines[1],88,"Component-wise vector %s.",c==0?"addition":"subtraction");out[1]=lines[1];aresult("Vector result",out,2);}
}
static void geo_construction(void)
{
    static const char *items[]={"Perpendicular bisector of a segment","Triangle angle-bisector theorem"};
    int c=amenu("Construction helper",items,2);double x1,y1,x2,y2,a,b,op;char lines[6][88];const char *out[6];
    if(c<0)return;
    if(c==0){
        if(!aread_number("A x",&x1)||!aread_number("A y",&y1)||!aread_number("B x",&x2)||!aread_number("B y",&y2))return;
        double mx=(x1+x2)/2,my=(y1+y2)/2,dx=x2-x1,dy=y2-y1;
        snprintf(lines[0],88,"Midpoint M = (%.8g, %.8g)",mx,my);out[0]=lines[0];
        if(fabs(dx)<1e-12)snprintf(lines[1],88,"AB is vertical; perpendicular bisector is horizontal.");else if(fabs(dy)<1e-12)snprintf(lines[1],88,"AB is horizontal; perpendicular bisector is vertical.");else snprintf(lines[1],88,"Perpendicular gradient = %.9g",-dx/dy);out[1]=lines[1];
        if(fabs(dy)<1e-12)snprintf(lines[2],88,"Perpendicular bisector: x = %.8g",mx);else if(fabs(dx)<1e-12)snprintf(lines[2],88,"Perpendicular bisector: y = %.8g",my);else {double m=-dx/dy;snprintf(lines[2],88,"Line: y = %.8g x %+.8g",m,my-m*mx);}out[2]=lines[2];
        snprintf(lines[3],88,"Construction: draw equal-radius arcs from A and B.");out[3]=lines[3];
        snprintf(lines[4],88,"Join the two arc intersections to make the bisector.");out[4]=lines[4];aresult("Perpendicular bisector",out,5);
    }else{
        if(!aread_number("Adjacent side a",&a)||!aread_number("Adjacent side b",&b)||!aread_number("Opposite side length",&op))return;
        if(a<=0||b<=0||op<=0){amessage("Angle bisector","Lengths must be positive","Enter positive side lengths.");return;}
        snprintf(lines[0],88,"Side split next to a: %.9g",op*a/(a+b));out[0]=lines[0];
        snprintf(lines[1],88,"Side split next to b: %.9g",op*b/(a+b));out[1]=lines[1];
        snprintf(lines[2],88,"Ratio of the two segments = a : b.");out[2]=lines[2];aresult("Angle-bisector theorem",out,3);
    }
}
static void geometry_menu(int choice)
{
    switch(choice){case 0:geo_bearings();break;case 1:geo_pythagoras();break;case 2:geo_trig();break;case 3:geo_area_volume();break;case 4:geo_similarity();break;case 5:geo_coordinates();break;case 6:geo_vectors();break;case 7:geo_construction();break;}
}

int rice_advanced_menu(int category)
{
    static const char *algebra[]={"Factorise polynomial (numerical real factors)","Simplify algebraic fractions","Rearrange common formulae","Complete the square"};
    static const char *graphs[]={"Solve polynomial roots (degree 1-8)","Find intersections","Find turning points","Generate a table of values","Analyse function transformations","Plot a polynomial"};
    static const char *stats[]={"Statistical summary","Frequency table","Histogram","Cumulative frequency","Box plot five-number summary","Regression and correlation"};
    static const char *geometry[]={"Angles and bearings","Pythagoras","Trigonometry (SOHCAHTOA)","Area and volume","Similarity and congruence","Coordinate geometry","Vector calculations","Construction helper"};
    const char **items=NULL;int n=0;const char *title="RICE tools";
    if(category==0){items=algebra;n=4;title="Advanced algebra";}
    else if(category==1){items=graphs;n=6;title="Equations and graph tools";}
    else if(category==2){items=stats;n=6;title="Statistics";}
    else if(category==3){items=geometry;n=8;title="Geometry";}
    else return 0;
    for(;;){int c=amenu(title,items,n);if(c<0)break;
        if(category==0)advanced_algebra(c);
        else if(category==1)equations_graphs(c);
        else if(category==2)stats_menu(c);
        else geometry_menu(c);
    }
    return 1;
}
