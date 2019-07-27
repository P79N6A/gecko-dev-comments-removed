



#ifndef _PEER_CONNECTION_TYPES_H_
#define _PEER_CONNECTION_TYPES_H_

#define MAX_TRACKS 8

typedef struct MediaTrack {
    unsigned int    media_stream_track_id;
    int             video;
} MediaTrack;

typedef struct MediaStreamTable {
    unsigned int    media_stream_id;
    unsigned int    num_tracks;
    MediaTrack      track[MAX_TRACKS];
} MediaStreamTable;


#endif 
