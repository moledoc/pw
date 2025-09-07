#pragma once

#ifndef PW_H_
#define PW_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MD5_IMPLEMENTATION
#include "md5.h"

#define MMALLOC_IMPLEMENTATION
#include "mmalloc.h"

char *pw(char *key, char *salt, char *pepper, char *domain, int length);
#endif // PW_H_

#ifdef PW_IMPLEMENTATION
char *pw(char *key, char *salt, char *pepper, char *domain, int length) {
    size_t domain_len = strlen(domain);
    size_t key_len = strlen(key);
    size_t salt_len = strlen(salt);
    size_t pepper_len = strlen(pepper);
    size_t message_len = domain_len + key_len + salt_len;

    char *message = mmalloc(sizeof(char)*message_len+1);

    memcpy(message, domain, domain_len);
    memcpy(message + domain_len, key, key_len);
    memcpy(message + domain_len + key_len, salt, salt_len);

    unsigned char digest[16];
    md5(message, digest);
    
    int max_pw_length = 16*2+pepper_len;
    char *pw = mmalloc(sizeof(char)*(max_pw_length+1));
    for (int i=0; i<16; ++i) {
        snprintf(pw+i*2, 2+1, "%02x", digest[i]);
    }

    if (0 < length && length < max_pw_length && pepper_len < length) {
        memset(pw+length-pepper_len, 0, max_pw_length-length+pepper_len);
        snprintf(pw+length-pepper_len, pepper_len+1, "%s", pepper);
    } else if (0 < length && length < max_pw_length && length < pepper_len) {
        memset(pw+length-1, 0, max_pw_length-length+1);
        snprintf(pw+length-1, 1+1, "%s", pepper);
    } else {
        snprintf(pw+16*2, pepper_len+1, "%s", pepper);
    }

    return pw;
}

#endif // PW_IMPLEMENTATION