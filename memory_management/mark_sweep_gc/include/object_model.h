#ifndef MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_OBJECT_MODEL_H

class MarkWord {

};

class KlassWord {

};

struct Header {
    MarkWord markWord;
    KlassWord kladdWord;
};

class Object {

private:
    Header header;
    void* obj;
};

#endif // MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_OBJECT_MODEL_H