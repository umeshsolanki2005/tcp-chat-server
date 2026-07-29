#include "auth.h"
#include <assert.h>

int main(void) {
    assert(auth_is_valid_username("alice") == true);
    assert(auth_is_valid_username("a") == false);
    assert(auth_is_valid_password("secret1") == true);
    assert(auth_is_valid_password("bad") == false);

    remove(USER_DB_PATH);
    assert(auth_register_user(USER_DB_PATH, "alice", "secret1") == 0);
    assert(auth_register_user(USER_DB_PATH, "alice", "other") == -1);
    assert(auth_login_user(USER_DB_PATH, "alice", "secret1") == 0);
    assert(auth_login_user(USER_DB_PATH, "alice", "wrong") == -1);
    return 0;
}
