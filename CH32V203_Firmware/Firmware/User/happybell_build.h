#ifndef HAPPYBELL_BUILD_H
#define HAPPYBELL_BUILD_H

/* The complete compact cooperative application is the production default.
 * Diagnostic and MP3-test profiles remain available for hardware service. */
#define HAPPYBELL_PROFILE_DIAGNOSTIC 0
#define HAPPYBELL_PROFILE_MP3_TEST   1
#define HAPPYBELL_PROFILE_FULL       2

/* The full application uses the cooperative foreground scheduler. */
#define HAPPYBELL_USE_FREERTOS       0

#ifndef HAPPYBELL_BUILD_PROFILE
#define HAPPYBELL_BUILD_PROFILE HAPPYBELL_PROFILE_FULL
#endif

#if HAPPYBELL_BUILD_PROFILE != HAPPYBELL_PROFILE_DIAGNOSTIC && \
    HAPPYBELL_BUILD_PROFILE != HAPPYBELL_PROFILE_MP3_TEST && \
    HAPPYBELL_BUILD_PROFILE != HAPPYBELL_PROFILE_FULL
#error "Invalid HAPPYBELL_BUILD_PROFILE"
#endif

#define HAPPYBELL_MP3_TEST_ENC_FOLDER "001"
#define HAPPYBELL_MP3_TEST_PLAIN_PATH "mp3_song.mp3"
/* Owner-selected full digital level. Plain and ENC audio use bounded automatic
 * gain and retain final saturation before PWM conversion. */
#define HAPPYBELL_MP3_TEST_VOLUME     255U

#endif
