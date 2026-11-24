#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "cpu.h"
#include "isa.h"
#include "files.h"



typedef enum {
    ST_FREE,
    ST_INSTR_FETCH,
    ST_ERROR,
    ST_FINISH
} PARSE_STAGE;


typedef enum {
    TK_INSTRUCTION,
    TK_REGISTER,
    TK_IMMEDIATE,
    TK_NEWLINE,
    TK_COMMA,
    TK_END,
    TK_BULLSHIT,
} TOKENTYPE;


struct instr_metadata {
    char name[10];
    TOKENTYPE template[4];
    int template_len;
};

static const struct instr_metadata instr_md_table[INSTR_END] = {
    [ADDI] = { "addi", { TK_REGISTER, TK_COMMA, TK_IMMEDIATE, TK_NEWLINE }, 4 },
    [ADD] = { "add", { TK_REGISTER, TK_COMMA, TK_REGISTER, TK_NEWLINE }, 4 }
};


typedef struct {
    TOKENTYPE type;
    union {
        int reg_number;
        int imm;
        INSTR instr_type;
    };

    int length;
} Token;



typedef struct {
    FileMap source, dest;

    PARSE_STAGE stage;

    INSTR target_instr;
    int instr_tpl_step;

    int current_line;
    int current_column;

    unsigned short instr_buffer;
    Token context_tokens[6];
    int context_tk_length;
} ParserContext;



Token get_token(char **cursor) {
    char *work_mem = *cursor;
    Token token;
    token.type = TK_BULLSHIT;

    for (;;work_mem++) {
        if (*work_mem != ' ') {
            break;
        }
    }

    token.length = 1;
    for (;;token.length++) {
        char symbol = work_mem[token.length];
        if (isspace(symbol) || symbol == '\0' || symbol == ',') {
            break;
        }
    }

    char symbol = *work_mem;

    if (symbol == '\0') {
        token.type = TK_END;
        return token;
    }


    char lookahead = *(work_mem+1);
    if (symbol == ',') {
        token.type = TK_COMMA;
    }
    else if (symbol == '\r' && lookahead == '\n') {
        token.type = TK_NEWLINE;
        token.length++;
    }
    // regs
    else if ((token.length == 2) && (symbol == 'x' || symbol == 'X') && (lookahead >= '0' && lookahead <= '8')) {
        token.type = TK_REGISTER;
        token.reg_number = lookahead - '0';
    }
    // immediate values 
    else if (symbol >= '0' && symbol <= '9') {
        char *bad_addr;
        int tmp = (int)strtol(work_mem, &bad_addr, 0);
        if (bad_addr != work_mem && isspace(*bad_addr)) {
            token.type = TK_IMMEDIATE;
            token.imm = tmp;
        }
    }
    // probably an instruction then
    else {
        for (int i = 0; i < INSTR_END; i++) {
            int len = strlen(instr_md_table[i].name);
            if (len == token.length && strncmp(work_mem, instr_md_table[i].name, token.length) == 0) {
                token.type = TK_INSTRUCTION;
                token.instr_type = i;
                break;
            }
        }
    }

    *cursor = work_mem + token.length;
    return token;
}


void stage_free(ParserContext *ctx, Token *tk);
void stage_instr_fetch(ParserContext *ctx, Token *tk);
void stage_finish(ParserContext *ctx, Token *tk);
void make_instruction_binary(ParserContext *ctx);

void assemble(const wchar_t *filename) {
    ParserContext ctx = {0};

    get_file(filename, &ctx.source, 'r', true);
    get_file(L"result.bin", &ctx.dest, 'w', false);

    char *cursor = ctx.source.map;

    ctx.stage = ST_FREE;
    ctx.current_line = 1;
    ctx.current_column = 1;

    while (ctx.stage != ST_FINISH && ctx.stage != ST_ERROR) {
        Token tk = get_token(&cursor);
        ctx.current_column += tk.length;

        if (tk.type == TK_NEWLINE) {
            ctx.current_column = 1;
            ctx.current_line++;
        }

        switch (ctx.stage) {
            case ST_FREE:
            stage_free(&ctx, &tk);
            break;

            case ST_INSTR_FETCH:
            stage_instr_fetch(&ctx, &tk);
            break;

            default:
            break;
            // case ST_FINISH:
            // stage_finish(&ctx, &tk);
            // break;
        }

    }
    if (ctx.stage == ST_FINISH) {
        printf("File assembled successfully. Lines read: %d:%d\n", ctx.current_line, ctx.current_column);
    }
    else {
        printf("File assembly failed due to the error above.\n");
    }

    close_file(&ctx.source);
    close_file(&ctx.dest);
}


void stage_free(ParserContext *ctx, Token *tk) {
    if (tk->type == TK_INSTRUCTION) {
        ctx->stage = ST_INSTR_FETCH;
        ctx->target_instr = tk->instr_type;
        ctx->instr_tpl_step = 0;
        ctx->context_tk_length = 0;
    }
    else if (tk->type == TK_END) {
        ctx->stage = ST_FINISH;
    }
    else if (tk->type == TK_BULLSHIT) {
        ctx->stage = ST_ERROR;
        printf("Unidentified symbol at line %d:%d\n", ctx->current_line, ctx->current_column);
    }
}


void stage_instr_fetch(ParserContext *ctx, Token *tk) {
    const struct instr_metadata *md = &instr_md_table[ctx->target_instr];

    bool last_instr = tk->type == TK_END? true : false;
    tk->type = last_instr? TK_NEWLINE : tk->type;

    if (tk->type == md->template[ctx->instr_tpl_step]) {
        ctx->context_tokens[ctx->instr_tpl_step] = *tk;
        ctx->instr_tpl_step++;

        if (ctx->instr_tpl_step == md->template_len) {
            ctx->stage = last_instr? ST_FINISH : ST_FREE;
            make_instruction_binary(ctx);
        }
        return;
    }
    printf("Instruction syntax error at line %d:%d\n", ctx->current_line, ctx->current_column);
    ctx->stage = ST_ERROR;
}


void stage_finish(ParserContext *ctx, Token *tk) {

}


void make_instruction_binary(ParserContext *ctx) {
    unsigned short buffer = 0;
    unsigned char bits_remaining = 16;

    bits_remaining -= 4;
    buffer |= (ctx->target_instr << bits_remaining);

    for (int i = 0; i < ctx->instr_tpl_step; i++) {
        Token *tok = &ctx->context_tokens[i];
        switch (tok->type) {
            case TK_REGISTER:
            bits_remaining -= 3;
            buffer |= (tok->reg_number << bits_remaining);
            break;

            case TK_IMMEDIATE:
            char num_bit_cnt = 32 - __builtin_clz(tok->imm);
            if (num_bit_cnt - bits_remaining > 0) {
                tok->imm >>= num_bit_cnt - bits_remaining;
            }
            buffer |= tok->imm;
            bits_remaining = 0;
            break;

            default:
            break;
        }
    }
    write_to_file(&ctx->dest, (char*)&buffer, sizeof(unsigned short));
}