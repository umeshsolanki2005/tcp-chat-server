#ifndef AUTH_H
#define AUTH_H

#include "common.h"

bool auth_is_valid_username(const char *username);
bool auth_is_valid_password(const char *password);
void auth_hash_password(const char *password, char *output, size_t output_size);
int auth_register_user(const char *db_path, const char *username, const char *password);
int auth_login_user(const char *db_path, const char *username, const char *password);
bool auth_username_exists(const char *db_path, const char *username);

#endif
