#ifndef _SCF_H_
#define _SCF_H_
#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////
// Note(pjako): scf is pseudo ini
//

typedef enum scf_Type {
    scf_type__invalid,
    scf_type_category,
    scf_type_num,
    scf_type_str,
    scf_type_true,
    scf_type_false,
} scf_Type;

typedef struct scf_Global {
    S8     fileName;
    S8     str;
    u64      line;
    u64      column;

    u64      needle;
    u64      nextOffset;

    S8     category;
    S8     key;
    scf_Type valueType;
    S8     valueStr;
} scf_Global;

API bool scf_next(scf_Global* scfGlobal);

API scf_Global scf_parse(S8 scfStr, S8 fileName);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _SCF_H_