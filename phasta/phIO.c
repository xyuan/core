#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcu_io.h>
#include <phIO.h>
#include <PCU.h>

#define PH_LINE 1024
#define MAGIC 362436
#define FIELD_PARAMS 3

enum {
NODES_PARAM,
VARS_PARAM,
STEP_PARAM
};

static const char* magic_name = "byteorder magic number";

void ph_write_header(FILE* f, const char* name, size_t bytes,
    int nparam, int* params)
{
  int i;
  fprintf(f,"%s : < %zu > ", name, bytes);
  for (i = 0; i < nparam; ++i)
    fprintf(f, "%d ", params[i]);
  fprintf(f, "\n");
}

static void skip_leading_spaces(char** s)
{
  while (**s == ' ') ++(*s);
}

static void cut_trailing_spaces(char* s)
{
  char* e = s + strlen(s);
  for (--e; e >= s; --e)
    if (*e != ' ')
      break;
  ++e;
  *e = '\0';
}

static void parse_header(char* header, char** name, size_t* bytes,
    int nparam, int* params)
{
  char* saveptr;
  int i;
  header = strtok_r(header, ":", &saveptr);
  if (name) {
    *name = header;
    skip_leading_spaces(name);
    cut_trailing_spaces(*name);
  }
  strtok_r(NULL, "<", &saveptr);
  header = strtok_r(NULL, ">", &saveptr);
  if (bytes)
    sscanf(header, "%zu", bytes);
  for (i = 0; i < nparam; ++i) {
    header = strtok_r(NULL, " \n", &saveptr);
    sscanf(header, "%d", &params[i]);
  }
}

static int find_header(FILE* f, const char* name, char header[PH_LINE])
{
  char* hname;
  size_t bytes;
  char tmp[PH_LINE];
  while (fgets(header, PH_LINE, f)) {
    if ((header[0] == '#') || (header[0] == '\n'))
      continue;
    strcpy(tmp, header);
    parse_header(tmp, &hname, &bytes, 0, NULL);
    if (!strcmp(name, hname))
      return 1;
    fseek(f, bytes, SEEK_CUR);
  }
  if (!PCU_Comm_Self())
    fprintf(stderr,"warning: phIO could not find \"%s\"\n",name);
  return 0;
}

static void write_magic_number(FILE* f)
{
  int why = 1;
  ph_write_header(f, magic_name, sizeof(int) + 1, 1, &why);
  int magic = MAGIC;
  fwrite(&magic, sizeof(int), 1, f);
  fprintf(f,"\n");
}

static int seek_after_header(FILE* f, const char* name)
{
  char dummy[PH_LINE];
  return find_header(f, name, dummy);
}

static void my_fread(void* p, size_t size, size_t nmemb, FILE* f)
{
  size_t r = fread(p, size, nmemb, f);
  assert(r == nmemb);
}

static int read_magic_number(FILE* f)
{
  if (!seek_after_header(f, magic_name)) {
    if (!PCU_Comm_Self())
      fprintf(stderr,"warning: not swapping bytes\n");
    rewind(f);
    return 0;
  }
  int magic;
  my_fread(&magic, sizeof(int), 1, f);
  return magic != MAGIC;
}

void ph_write_preamble(FILE* f)
{
  fprintf(f, "# PHASTA Input File Version 2.0\n");
  fprintf(f, "# Byte Order Magic Number : 362436 \n");
  fprintf(f, "# Output generated by libph version: yes\n");
  write_magic_number(f);
}

void ph_write_doubles(FILE* f, const char* name, double* data,
    size_t n, int nparam, int* params)
{
  ph_write_header(f, name, n * sizeof(double) + 1, nparam, params);
  fwrite(data, sizeof(double), n, f);
  fprintf(f, "\n");
}

void ph_write_ints(FILE* f, const char* name, int* data,
    size_t n, int nparam, int* params)
{
  ph_write_header(f, name, n * sizeof(int) + 1, nparam, params);
  fwrite(data, sizeof(int), n, f);
  fprintf(f, "\n");
}

static void parse_params(char* header, size_t* bytes,
    int* nodes, int* vars, int* step)
{
  int params[FIELD_PARAMS];
  parse_header(header, NULL, bytes, FIELD_PARAMS, params);
  *nodes = params[NODES_PARAM];
  *vars = params[VARS_PARAM];
  *step = params[STEP_PARAM];
}

void ph_read_field(const char* file, const char* field, double** data,
    int* nodes, int* vars, int* step)
{
  size_t bytes, n;
  char header[PH_LINE];
  int should_swap;
  int ok;
  FILE* f = fopen(file, "r");
  if (!f) {
    fprintf(stderr,"could not open \"%s\"\n", file);
    abort();
  }
  should_swap = read_magic_number(f);
  ok = find_header(f, field, header);
  assert(ok);
  parse_params(header, &bytes, nodes, vars, step);
  assert(((bytes - 1) % sizeof(double)) == 0);
  n = (bytes - 1) / sizeof(double);
  assert(n == (*nodes) * (*vars));
  *data = malloc(bytes);
  my_fread(*data, sizeof(double), n, f);
  if (should_swap)
    pcu_swap_doubles(*data, n);
  fclose(f);
}

void ph_write_field(FILE* f, const char* field, double* data,
    int nodes, int vars, int step)
{
  int params[FIELD_PARAMS];
  params[NODES_PARAM] = nodes;
  params[VARS_PARAM] = vars;
  params[STEP_PARAM] = step;
  ph_write_doubles(f, field, data, nodes * vars, FIELD_PARAMS, params);
}
