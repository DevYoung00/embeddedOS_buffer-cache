#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

/* Stack 관련 */
Stack* init_stack() {
    Stack* s = (Stack*)malloc(sizeof(Stack));
    for (int i = 0; i < STACK_SIZE; i++) {
        s->items[i] = 0;
    }
    s->top = -1;
    return s;
}

void free_stack(Stack* s) {
    free(s);
}
int is_value_in_stack(Stack* s, int value) {
    for (int i = 0; i <= s->top; ++i) {
        if (s->items[i] == value) {
            return 1;  // 찾고자 하는 값이 스택에 존재함 (true)
        }
    }
    return 0;  // 찾고자 하는 값이 스택에 존재하지 않음 (false)
}


void push(Stack* s, int block_nr) {
    // 이미 스택에 값이 존재하면 해당 값을 제거
    if (is_value_in_stack(s, block_nr)) {
        for (int i = 0; i <= s->top; ++i) {
            if (s->items[i] == block_nr) {
                for (int j = i; j < s->top; ++j) {
                    s->items[j] = s->items[j + 1];
                }
                --(s->top);
                break;
            }
        }
    }

    // 맨 위로 push
    if (s->top < STACK_SIZE - 1) {
        s->items[++(s->top)] = block_nr;
    }
    //  printf("Stack push: ");
    // for (int i = 0; i <= s->top; ++i) {
    //     printf("%d ", s->items[i]);
    // }
    // printf("\n");
}

int pop(Stack* s) {
    // if (s->top >= 0) {
    //      printf("Stack pop: ");
    // for (int i = 0; i <= s->top; ++i) {
    //     printf("%d ", s->items[i]);
    // }
    // printf("\n");
        return s->items[(s->top)--];

    //}
    return -1; // Stack is empty
}

