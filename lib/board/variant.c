#include "keepkey/board/variant.h"

#include "keepkey/board/keepkey_flash.h"
#include "keepkey/board/pubkeys.h"
#include "keepkey/crypto/secp256k1.h"
#include "keepkey/crypto/ecdsa.h"
#include "keepkey/crypto/sha2.h"
#include "keepkey/variant/keepkey.h"
#include "keepkey/variant/salt.h"

#include <string.h>

#define SIGNEDVARIANTINFO_FLASH (SignedVariantInfo*)(0x8010000)

static const VariantAnimation *screensaver;
static const VariantAnimation *logo;
static const VariantAnimation *logo_reversed;
static const char *name;
static const uint32_t *screensaver_timeout;

// Retrieves model information from storage
Model getModel(void) {

    const char *model = flash_getModel();
    if (!model)
	return MODEL_UNKNOWN;
#define MODEL_ENTRY_KK(STRING, ENUM) \
    if (0 == strcmp(model, (STRING))) { \
        return MODEL_KEEPKEY; \
    }
#define MODEL_ENTRY_SALT(STRING, ENUM) \
    if (0 == strcmp(model, (STRING))) { \
        return MODEL_SALT; \
    }
#define MODEL_ENTRY_FOX(STRING, ENUM) \
    if (0 == strcmp(model, (STRING))) { \
	return MODEL_FOX; \
    }
#include "keepkey/board/models.def"

    return MODEL_UNKNOWN;
}

const VariantInfo * __attribute__((weak)) variant_getInfo(void) {

    switch (getModel()) {
    case MODEL_KEEPKEY: return &variant_keepkey;
    case MODEL_SALT: return &variant_salt;
    case MODEL_FOX: return &variant_keepkey;
    case MODEL_UNKNOWN: return &variant_keepkey;
    }
    return &variant_keepkey;
}

const VariantAnimation *variant_getScreensaver(void) {
    if (screensaver)
        return screensaver;

    screensaver = variant_getInfo()->screensaver;
    return screensaver;
}

const VariantAnimation *variant_getLogo(bool reversed) {
    if (reversed && logo_reversed)
        return logo_reversed;
    if (!reversed && logo)
        return logo;

    logo = variant_getInfo()->logo;
    logo_reversed = variant_getInfo()->logo_reversed;

    return reversed ? logo_reversed : logo;
}

const char *variant_getName(void) {
    if (name) {
        return name;
    }

    name = variant_getInfo()->name;
    return name;
}

uint32_t variant_getScreensaverTimeout(void) {
    if (screensaver_timeout) {
        return *screensaver_timeout;
    }

    screensaver_timeout = &variant_getInfo()->screensaver_timeout;
    return *screensaver_timeout;
}
