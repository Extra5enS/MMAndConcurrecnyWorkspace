
#ifndef MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_TABLE_H
#define MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_TABLE_H

// need to implement singleton that will be used to create and save info about classes

#include <string_view>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>

class Klass {
    void VisitAllClassFields(void* obj, std::function<void(void*)> callback);
    void* GetOffsetByIndex(size_t index);
private:
    std::vector<size_t> fieldsOffsets;
};

using KlassId = uint32_t;

class KlassTable {

    static constexpr auto CLASS_NAME_DECL = "struct"; 
    static constexpr auto CLASS_NAME_REG = "(\\w+)";

    static constexpr auto FIELD_TYPE_REG = "(word)|(obj)";
    static constexpr auto FIELD_NAME_REG = "(\\w+)";

public:
    static KlassTable* Create();
    static KlassTable* GetCurrent();
    static void Delete();

    KlassId ParseClass(const std::string& classString);
    Klass& GetClassById(KlassId id);
    Klass*& GetClassByName(std::string_view name);
    KlassId GetIdByName(std::string_view name);

private:
    KlassTable() = default;

    static KlassTable* inst_;

    std::unordered_map<std::string, KlassId> nameToId_;
    std::unordered_map<KlassId, Klass*> idToKlass_;

};

#endif // MEMORY_MANAGEMENT_OBJECT_MODEL_INCLUDE_KLASS_TABLE_H