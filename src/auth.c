#include "auth.h"

static int auth_append_user(const char *db_path, const char *username, const char *password_hash) {
    FILE *file = fopen(db_path, "a");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    fprintf(file, "%s:%s\n", username, password_hash);
    fclose(file);
    return 0;
}

bool auth_is_valid_username(const char *username) {
    size_t i;
    size_t length;

    if (username == NULL) {
        return false;
    }

    length = strlen(username);
    if (length < 3 || length > 20) {
        return false;
    }

    for (i = 0; i < length; ++i) {
        char ch = username[i];
        if (!isalnum((unsigned char)ch) && ch != '_') {
            return false;
        }
    }

    return true;
}

bool auth_is_valid_password(const char *password) {
    size_t length;

    if (password == NULL) {
        return false;
    }

    length = strlen(password);
    if (length < 6 || length > 64) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        if (password[i] == '\n' || password[i] == '\r') {
            return false;
        }
    }

    return true;
}

void auth_hash_password(const char *password, char *output, size_t output_size) {
    unsigned long hash = 5381;
    size_t i;

    if (password == NULL || output == NULL || output_size == 0) {
        if (output != NULL && output_size > 0) {
            output[0] = '\0';
        }
        return;
    }

    for (i = 0; password[i] != '\0'; ++i) {
        hash = ((hash << 5) + hash) + (unsigned long)(unsigned char)password[i];
    }

    snprintf(output, output_size, "%lu", hash);
}

bool auth_username_exists(const char *db_path, const char *username) {
    FILE *file = fopen(db_path, "r");
    char line[MAX_LINE_LENGTH];

    if (file == NULL) {
        return false;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *colon = strchr(line, ':');
        if (colon != NULL) {
            *colon = '\0';
            if (strcmp(line, username) == 0) {
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return false;
}

int auth_register_user(const char *db_path, const char *username, const char *password) {
    char password_hash[MAX_PASSWORD_HASH];

    if (!auth_is_valid_username(username) || !auth_is_valid_password(password)) {
        errno = EINVAL;
        return -1;
    }

    if (auth_username_exists(db_path, username)) {
        errno = EEXIST;
        return -1;
    }

    auth_hash_password(password, password_hash, sizeof(password_hash));
    return auth_append_user(db_path, username, password_hash);
}

int auth_login_user(const char *db_path, const char *username, const char *password) {
    FILE *file = fopen(db_path, "r");
    char line[MAX_LINE_LENGTH];
    char expected_hash[MAX_PASSWORD_HASH];

    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    auth_hash_password(password, expected_hash, sizeof(expected_hash));

    while (fgets(line, sizeof(line), file) != NULL) {
        char *colon = strchr(line, ':');
        char *hash = NULL;
        if (colon != NULL) {
            *colon = '\0';
            hash = colon + 1;
            hash[strcspn(hash, "\r\n")] = '\0';
            if (strcmp(line, username) == 0 && strcmp(hash, expected_hash) == 0) {
                fclose(file);
                return 0;
            }
        }
    }

    fclose(file);
    return -1;
}
