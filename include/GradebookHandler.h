#ifndef AA_GRADEBOOK_HANDLER_H
#define AA_GRADEBOOK_HANDLER_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Gradebook.h"
#include "RequestHandler.h"
#include "sql/gradebook.h"

namespace AA::Server::Request {

class GradebookHandler final : public Handler {
 public:
  using Handler::Handler;

 protected:
  std::string_view entityName() const override { return "gradebook"; }

  nlohmann::json executeImpl(const std::string& action,
                             const nlohmann::json& payload,
                             mysqlpp::Connection& conn) override {
    AA::Data::gradebook repo{conn};

    // Base DataBase CRUD operations
    if (action == "create" || action == "save") {
      return handleCreate(repo, payload);
    }
    if (action == "read" || action == "get") {
      return handleRead(repo, payload);
    }
    if (action == "update") {
      return handleUpdate(repo, payload);
    }
    if (action == "delete" || action == "remove") {
      return handleDelete(repo, payload);
    }

    // Custom AA::Data::gradebook operations
    if (action == "load_all" || action == "get_all" || action == "list") {
      return handleLoadAll(repo);
    }
    if (action == "load_for_school" || action == "get_for_school") {
      return handleLoadForSchool(repo, payload);
    }
    if (action == "update_for_school" || action == "set_for_school") {
      return handleUpdateForSchool(repo, payload);
    }

    return error(action, "Unsupported gradebook action: " + action);
  }

 private:
  static AA::Gradebook extractGradebook(const nlohmann::json& payload) {
    if (payload.contains("gradebook") && payload["gradebook"].is_object()) {
      return payload.at("gradebook").get<AA::Gradebook>();
    }
    return payload.get<AA::Gradebook>();
  }

  static std::string resolveIdColumn(const nlohmann::json& payload) {
    if (payload.contains("id_column") && !payload["id_column"].is_null()) {
      const std::string col = requireField<std::string>(payload, "id_column");
      if (col != "name") {
        throw std::invalid_argument("Unsupported id_column: " + col);
      }
      return col;
    }
    return "name";
  }

  static std::string resolveIdValue(const nlohmann::json& payload,
                                    const std::string& idColumn,
                                    bool allowRenameKeys = false) {
    if (allowRenameKeys) {
      for (const char* key : {"old_name", "target_name"}) {
        if (payload.contains(key) && !payload[key].is_null()) {
          return requireField<std::string>(payload, key);
        }
      }
    }

    if (payload.contains(idColumn) && !payload[idColumn].is_null()) {
      return requireField<std::string>(payload, idColumn);
    }
    if (payload.contains("gradebook") && payload["gradebook"].is_object()) {
      const auto& nested = payload.at("gradebook");
      if (nested.contains(idColumn) && !nested[idColumn].is_null()) {
        return requireField<std::string>(nested, idColumn);
      }
    }
    return requireField<std::string>(payload, "name");
  }

  static std::string resolveSchoolName(const nlohmann::json& payload) {
    if (payload.contains("school_name") && !payload["school_name"].is_null()) {
      return requireField<std::string>(payload, "school_name");
    }
    if (payload.contains("gradebook") && payload["gradebook"].is_object()) {
      const auto& nested = payload.at("gradebook");
      if (nested.contains("school_name") && !nested["school_name"].is_null()) {
        return requireField<std::string>(nested, "school_name");
      }
    }
    throw std::invalid_argument("Missing required field: school_name");
  }

  // --- Base DataBase CRUD ---

  nlohmann::json handleCreate(AA::Data::gradebook& repo,
                              const nlohmann::json& payload) const {
    const AA::Gradebook gb = extractGradebook(payload);
    if (!repo.save(gb)) {
      return error("create", "Failed to save gradebook record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["gradebook"] = gb;
    return ok("create", std::move(result));
  }

  nlohmann::json handleRead(AA::Data::gradebook& repo,
                            const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Gradebook gb{};
    if (!repo.read(idColumn, idValue, gb)) {
      return error("read", "Gradebook not found");
    }

    nlohmann::json result = nlohmann::json::object();
    result["gradebook"] = gb;
    return ok("read", std::move(result));
  }

  nlohmann::json handleUpdate(AA::Data::gradebook& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue =
        resolveIdValue(payload, idColumn, /*allowRenameKeys=*/true);

    AA::Gradebook existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("update", "Gradebook not found");
    }

    const AA::Gradebook updated = extractGradebook(payload);
    if (!repo.update(updated, idColumn, idValue)) {
      return error("update", "Failed to update gradebook record");
    }

    nlohmann::json result = nlohmann::json::object();
    result["gradebook"] = updated;
    return ok("update", std::move(result));
  }

  nlohmann::json handleDelete(AA::Data::gradebook& repo,
                              const nlohmann::json& payload) const {
    const std::string idColumn = resolveIdColumn(payload);
    const std::string idValue = resolveIdValue(payload, idColumn);

    AA::Gradebook existing{};
    if (!repo.read(idColumn, idValue, existing)) {
      return error("delete", "Gradebook not found");
    }

    if (!repo.remove(idColumn, idValue)) {
      return error("delete", "Failed to delete gradebook record");
    }
    return ok("delete", {{"deleted", true}, {idColumn, idValue}});
  }

  // --- Custom AA::Data::gradebook Methods ---

  nlohmann::json handleLoadAll(const AA::Data::gradebook& repo) const {
    std::vector<std::pair<std::string, AA::Gradebook>> entries;
    if (!repo.loadAllGradeBooks(entries)) {
      return error("load_all", "Failed to load gradebooks");
    }

    nlohmann::json items = nlohmann::json::array();
    for (const auto& [schoolName, gb] : entries) {
      nlohmann::json item = nlohmann::json::object();
      item["school_name"] = schoolName;
      item["gradebook"] = gb;
      items.push_back(std::move(item));
    }

    nlohmann::json result = nlohmann::json::object();
    result["gradebooks"] = std::move(items);
    return ok("load_all", std::move(result));
  }

  nlohmann::json handleLoadForSchool(const AA::Data::gradebook& repo,
                                     const nlohmann::json& payload) const {
    const std::string schoolName = resolveSchoolName(payload);

    AA::Gradebook gb{};
    if (!repo.loadGradeBookForSchool(schoolName, gb)) {
      return error("load_for_school", "Failed to load gradebook for school");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school_name"] = schoolName;
    result["gradebook"] = gb;
    return ok("load_for_school", std::move(result));
  }

  nlohmann::json handleUpdateForSchool(AA::Data::gradebook& repo,
                                       const nlohmann::json& payload) const {
    const std::string schoolName = resolveSchoolName(payload);
    const AA::Gradebook gb = extractGradebook(payload);

    if (!repo.updateGradeBookForSchool(schoolName, gb)) {
      return error("update_for_school",
                   "Failed to update gradebook for school");
    }

    nlohmann::json result = nlohmann::json::object();
    result["school_name"] = schoolName;
    result["gradebook"] = gb;
    return ok("update_for_school", std::move(result));
  }
};

}  // namespace AA::Server::Request

#endif  // AA_SERVER_REQUEST_GRADEBOOK_HANDLER_H
