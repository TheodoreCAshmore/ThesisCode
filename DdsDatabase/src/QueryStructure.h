/**
 * @file QueryStructure.h
 * @author Saül Abad Copoví
 * @brief Creates structs for every kind of results given by the queries
 */
#include <string>
#include <vector>

#pragma once

namespace QueryStructure
{
    /**
     * @brief Structure used by queries of type Query01, Query04, Query09 and Query11. Includes comparison operators.
     *
     */
    struct structure_generic
    {
        int max_id;
        int compaction_prev_revision;
        int theid;
        std::string name;
        int created;
        int deleted;
        int create_revision;
        int prev_revision;
        int lease;
        std::vector<uint8_t> value;
        std::vector<uint8_t> old_value;

        /**
         * @brief Construct a new empty structure generic object
         *
         */
        structure_generic()
            : max_id(0), compaction_prev_revision(0),
              theid(0), name(""), created(0), deleted(0), create_revision(0), prev_revision(0), lease(0),
              value(std::vector<uint8_t>()), old_value(std::vector<uint8_t>())
        {
        }

        /**
         * @brief Construct a new structure generic object
         *
         * @param mid Maximum id
         * @param cpr Compaction previous revision
         * @param tid The id field of the message
         * @param n Name field of the message
         * @param c Created field of the message
         * @param d Deleted field of the message
         * @param cr Create_revision field of the message
         * @param pr Prev_revision field of the message
         * @param l Lease field of the message
         * @param v Value field of the message
         * @param ov Old_value field of the message
         */
        structure_generic(int mid, int cpr, int tid, std::string n, int c, int d, int cr, int pr, int l, std::vector<uint8_t> v, std::vector<uint8_t> ov)
            : max_id(mid), compaction_prev_revision(cpr),
              theid(tid), name(n),
              created(c), deleted(d),
              create_revision(cr), prev_revision(pr),
              lease(l),
              value(v), old_value(ov)
        {
        }

        /**
         * @brief Overload of the < operator.
         *
         * @param l Left side element
         * @param r Right side element
         * @return true
         * @return false
         */
        friend bool operator<(const QueryStructure::structure_generic &l, const QueryStructure::structure_generic &r)
        {
            return std::tie(l.max_id, l.compaction_prev_revision, l.theid, l.name, l.created, l.deleted,
                            l.create_revision, l.prev_revision, l.lease, l.value, l.old_value) <
                   std::tie(r.max_id, r.compaction_prev_revision, r.theid, r.name, r.created, r.deleted,
                            r.create_revision, r.prev_revision, r.lease, r.value, r.old_value);
        }

        /**
         * @brief Overload of the == operator
         *
         * @param l Left side element
         * @param r Right side element
         * @return true
         * @return false
         */
        friend bool operator==(const QueryStructure::structure_generic &l, const QueryStructure::structure_generic &r)
        {
            return std::tie(l.max_id, l.compaction_prev_revision, l.theid, l.name, l.created, l.deleted,
                            l.create_revision, l.prev_revision, l.lease, l.value, l.old_value) ==
                   std::tie(r.max_id, r.compaction_prev_revision, r.theid, r.name, r.created, r.deleted,
                            r.create_revision, r.prev_revision, r.lease, r.value, r.old_value);
        }
    };

    /**
     * @brief Structure used by queries of type Query03. Includes comparison operators.
     *
     */
    struct structure_query03
    {
        int id;
        int count;

        /**
         * @brief Construct a new empty structure query03 object
         *
         */
        structure_query03()
            : id(0), count(0)
        {
        }

        /**
         * @brief Construct a new structure query03 object
         *
         * @param id Maximum id
         * @param count count
         */
        structure_query03(int id, int count)
            : id(id), count(count)
        {
        }
    };

    struct structure_massDisposeCompaction
    {
        bool success;
        int rows_disposed;

        structure_massDisposeCompaction()
            : success(false), rows_disposed(0)
        {
        }

        structure_massDisposeCompaction(bool success, int rows_disposed)
            : success(success), rows_disposed(rows_disposed)
        {
        }
    };

}
