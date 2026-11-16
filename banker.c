#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

static int m, n;
static int *mx, *al, *av;
static FILE *out;

#define MX(i, j) mx[(i) * m + (j)]
#define AL(i, j) al[(i) * m + (j)]

struct cmd {
    char t;
    int c;
    int *r;
};

static void die(const char *f, ...)
{
    va_list a;
    va_start(a, f);
    fputs("erro: ", stderr);
    vfprintf(stderr, f, a);
    fputc('\n', stderr);
    va_end(a);
    exit(1);
}

static int num(const char *s, int *v)
{
    char *e;
    long x;
    if (!isdigit((unsigned char)*s))
        return 0;
    x = strtol(s, &e, 10);
    if (*e || x > 1000000)
        return 0;
    *v = (int)x;
    return 1;
}

static char *trim(char *s)
{
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

static void vec(const int *v)
{
    int j;
    for (j = 0; j < m; j++)
        fprintf(out, j ? " %d" : "%d", v[j]);
}

static void customers(void)
{
    FILE *f = fopen("customer.txt", "r");
    char buf[1024];
    int cap = 8, ln = 0;
    if (!f)
        die("nao abriu customer.txt");
    mx = malloc(cap * m * sizeof(int));
    while (fgets(buf, sizeof buf, f)) {
        char *s = trim(buf), *c;
        int j = 0;
        ln++;
        if (!*s)
            continue;
        if (n == cap) {
            cap *= 2;
            mx = realloc(mx, cap * m * sizeof(int));
        }
        for (;;) {
            c = strchr(s, ',');
            if (c)
                *c = 0;
            if (j >= m || !num(s, &MX(n, j)))
                die("customer.txt linha %d: formato invalido (esperado %d colunas)", ln, m);
            j++;
            if (!c)
                break;
            s = c + 1;
        }
        if (j != m)
            die("customer.txt linha %d: %d colunas, esperado %d", ln, j, m);
        n++;
    }
    fclose(f);
    if (!n)
        die("customer.txt vazio");
}

static struct cmd *commands(int *k)
{
    FILE *f = fopen("commands.txt", "r");
    char buf[1024], **tk;
    struct cmd *v = NULL;
    int cap = 0, ln = 0, j, t;
    *k = 0;
    if (!f)
        die("nao abriu commands.txt");
    tk = malloc((m + 3) * sizeof(char *));
    while (fgets(buf, sizeof buf, f)) {
        char *s = trim(buf);
        struct cmd *x;
        ln++;
        if (!*s)
            continue;
        t = 0;
        for (s = strtok(s, " \t"); s && t < m + 3; s = strtok(NULL, " \t"))
            tk[t++] = s;
        if (*k == cap) {
            cap = cap ? cap * 2 : 16;
            v = realloc(v, cap * sizeof *v);
        }
        x = &v[*k];
        x->r = NULL;
        x->c = -1;
        if (!strcmp(tk[0], "*")) {
            if (t != 1)
                die("commands.txt linha %d: '*' sozinho na linha", ln);
            x->t = '*';
        } else if (!strcmp(tk[0], "RQ") || !strcmp(tk[0], "RL")) {
            x->t = tk[0][1];
            if (t != m + 2)
                die("commands.txt linha %d: esperado %d valores de recurso", ln, m);
            if (!num(tk[1], &x->c) || x->c >= n)
                die("commands.txt linha %d: cliente invalido", ln);
            x->r = malloc(m * sizeof(int));
            for (j = 0; j < m; j++)
                if (!num(tk[j + 2], &x->r[j]))
                    die("commands.txt linha %d: recurso invalido", ln);
        } else
            die("commands.txt linha %d: comando desconhecido", ln);
        (*k)++;
    }
    free(tk);
    fclose(f);
    return v;
}

static int safe(void)
{
    int *w = malloc(m * sizeof(int)), *ok = calloc(n, sizeof(int));
    int i, j, done = 0, moved = 1;
    memcpy(w, av, m * sizeof(int));
    while (moved) {
        moved = 0;
        for (i = 0; i < n; i++) {
            if (ok[i])
                continue;
            for (j = 0; j < m && MX(i, j) - AL(i, j) <= w[j]; j++)
                ;
            if (j < m)
                continue;
            for (j = 0; j < m; j++)
                w[j] += AL(i, j);
            ok[i] = moved = 1;
            done++;
        }
    }
    free(w);
    free(ok);
    return done == n;
}

static void request(int c, int *r)
{
    int j;
    for (j = 0; j < m; j++)
        if (r[j] > MX(c, j) - AL(c, j))
            break;
    if (j < m) {
        fprintf(out, "The customer %d request ", c);
        vec(r);
        fputs(" was denied because exceed its maximum allocation\n", out);
        return;
    }
    for (j = 0; j < m; j++)
        if (r[j] > av[j])
            break;
    if (j < m) {
        fputs("The resources ", out);
        vec(av);
        fprintf(out, " are not enough to customer %d request ", c);
        vec(r);
        fputc('\n', out);
        return;
    }
    for (j = 0; j < m; j++) {
        av[j] -= r[j];
        AL(c, j) += r[j];
    }
    if (!safe()) {
        for (j = 0; j < m; j++) {
            av[j] += r[j];
            AL(c, j) -= r[j];
        }
        fprintf(out, "The customer %d request ", c);
        vec(r);
        fputs(" was denied because result in an unsafe state\n", out);
        return;
    }
    fprintf(out, "Allocate to customer %d the resources ", c);
    vec(r);
    fputc('\n', out);
}

static void release(int c, int *r)
{
    int j;
    for (j = 0; j < m; j++)
        if (r[j] > AL(c, j))
            break;
    if (j < m) {
        fprintf(out, "The customer %d release ", c);
        vec(r);
        fputs(" was denied because exceed its allocation\n", out);
        return;
    }
    for (j = 0; j < m; j++) {
        av[j] += r[j];
        AL(c, j) -= r[j];
    }
    fprintf(out, "Release from customer %d the resources ", c);
    vec(r);
    fputc('\n', out);
}

static void state(void)
{
    int i, j, w1 = 2 * m + 4 > 10 ? 2 * m + 4 : 10, w2 = 2 * m + 6 > 12 ? 2 * m + 6 : 12;
    char *a = malloc(m * 12 + 2), *b = malloc(m * 12 + 2), *d = malloc(m * 12 + 2);
    fprintf(out, "%-*s|   %-*s|   NEED\n", w1, "MAXIMUM", w2, "ALLOCATION");
    for (i = 0; i < n; i++) {
        char *pa = a, *pb = b, *pd = d;
        for (j = 0; j < m; j++) {
            pa += sprintf(pa, "%d ", MX(i, j));
            pb += sprintf(pb, "%d ", AL(i, j));
            pd += sprintf(pd, "%d ", MX(i, j) - AL(i, j));
        }
        fprintf(out, "%-*s|   %-*s|   %s\n", w1, a, w2, b, d);
    }
    fputs("AVAILABLE ", out);
    for (j = 0; j < m; j++)
        fprintf(out, "%d ", av[j]);
    fputc('\n', out);
    free(a);
    free(b);
    free(d);
}

int main(int argc, char **argv)
{
    struct cmd *v;
    int i, k;
    if (argc < 2)
        die("uso: %s r1 r2 ...", argv[0]);
    m = argc - 1;
    av = malloc(m * sizeof(int));
    for (i = 0; i < m; i++)
        if (!num(argv[i + 1], &av[i]))
            die("argumento '%s' invalido", argv[i + 1]);
    customers();
    v = commands(&k);
    al = calloc(n * m, sizeof(int));
    out = fopen("result.txt", "w");
    if (!out)
        die("nao criou result.txt");
    for (i = 0; i < k; i++) {
        if (v[i].t == 'Q')
            request(v[i].c, v[i].r);
        else if (v[i].t == 'L')
            release(v[i].c, v[i].r);
        else
            state();
    }
    fclose(out);
    return 0;
}