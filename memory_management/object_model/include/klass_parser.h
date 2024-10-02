
#ifndef MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_PARSER_H
#define MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_PARSER_H

// need to implement singleton that will be used to create and save info about classes

#include <string_view>
class KlassParser {

    static KlassParser* Create();
    static KlassParser* GetCurrent();
    static void Delete();

    size_t AddClass(std::string_view klass);
    

private:
    KlassParser() = default;

    static KlassParser* inst_;
};

#endif // MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_PARSER_H