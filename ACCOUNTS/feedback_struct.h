#ifndef FEEDBACK_RECORD
#define FEEDBACK_RECORD

struct Feedback
{
    int ID;
    char buffer[500];
    bool isreview;//true:reviewed,false:not reviewed

};

#endif