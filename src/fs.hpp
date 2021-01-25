// Copyright (C) 2021 jeliebig
// Copyright (C) 2020 averne
//
// This file is part of Turnips-Save-Manager (This is a modified version of Turnips).
//
// Turnips is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Turnips is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Turnips.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <switch.h>

namespace fs {

struct Directory {
    FsDir handle = {};

    constexpr inline Directory() = default;
    constexpr inline Directory(const FsDir &handle): handle(handle) { }

    inline ~Directory() {
        this->close();
    }

    inline Result open(FsFileSystem *fs, const std::string &path) {
        auto tmp = path;
        tmp.reserve(FS_MAX_PATH); // Fails otherwise
        return fsFsOpenDirectory(fs, tmp.c_str(), FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &this->handle);
    }

    inline void close() {
        fsDirClose(&this->handle);
    }

    inline bool isOpen() const {
        // TODO: Better heuristic?
        return this->handle.s.session;
    }

    inline std::size_t count() {
        std::int64_t count = 0;
        fsDirGetEntryCount(&this->handle, &count);
        return count;
    }

    std::vector<FsDirectoryEntry> list() {
        std::int64_t total = 0;
        auto c = this->count();
        auto entries = std::vector<FsDirectoryEntry>(c);
        fsDirRead(&this->handle, &total, c, entries.data());
        return entries;
    }
};

struct File {
    FsFile handle = {};

    constexpr inline File() = default;
    constexpr inline File(const FsFile &handle): handle(handle) { }

    inline ~File() {
        this->close();
    }

    inline Result open(FsFileSystem *fs, const std::string &path, std::uint32_t mode = FsOpenMode_Read) {
        auto tmp = path;
        tmp.reserve(FS_MAX_PATH);
        return fsFsOpenFile(fs, tmp.c_str(), mode, &this->handle);
    }

    inline void close() {
        fsFileClose(&this->handle);
    }

    inline bool isOpen() const {
        return this->handle.s.session;
    }

    inline std::size_t size() {
        std::int64_t tmp = 0;
        fsFileGetSize(&this->handle, &tmp);
        return tmp;
    }

    inline void size(std::size_t size) {
        fsFileSetSize(&this->handle, static_cast<std::int64_t>(size));
    }

    inline std::size_t read(void *buf, std::size_t size, std::size_t offset = 0) {
        std::uint64_t tmp = 0;
        auto rc = fsFileRead(&this->handle, static_cast<std::int64_t>(offset), buf, static_cast<std::uint64_t>(size), FsReadOption_None, &tmp);
        if (R_FAILED(rc))
            printf("Read failed with %#x\n", rc);
        return tmp;
    }

    inline void write(const void *buf, std::size_t size, std::size_t offset = 0) {
        fsFileWrite(&this->handle, static_cast<std::int64_t>(offset), buf, size, FsWriteOption_None);
    }

    inline void flush() {
        fsFileFlush(&this->handle);
    }
};

struct Filesystem {
    FsFileSystem handle = {};

    constexpr inline Filesystem() = default;
    constexpr inline Filesystem(const FsFileSystem &handle): handle(handle) { }

    inline ~Filesystem() {
        this->close();
    }

    inline Result open(FsBisPartitionId id) {
        return fsOpenBisFileSystem(&this->handle, id, "");
    }

    inline Result openSDMC() {
        return fsOpenSdCardFileSystem(&this->handle);
    }

    inline void close() {
        flush();
        fsFsClose(&this->handle);
    }

    inline bool isOpen() const {
        return this->handle.s.session;
    }

    inline Result flush() {
        return fsFsCommit(&this->handle);
    }

    inline std::size_t getTotalSpace() {
        std::int64_t tmp = 0;
        fsFsGetTotalSpace(&this->handle, "/", &tmp);
        return tmp;
    }

    inline std::size_t getFreeSpace() {
        std::int64_t tmp = 0;
        fsFsGetFreeSpace(&this->handle, "/", &tmp);
        return tmp;
    }

    inline Result openDirectory(Directory &d, const std::string &path) {
        return d.open(&this->handle, path);
    }

    inline Result openFile(File &f, const std::string &path, std::uint32_t mode = FsOpenMode_Read) {
        return f.open(&this->handle, path, mode);
    }

    inline Result createDirectory(const std::string &path) {
        return fsFsCreateDirectory(&this->handle, path.c_str());
    }

    inline Result createFile(const std::string &path, std::size_t size = 0) {
        return fsFsCreateFile(&this->handle, path.c_str(), static_cast<std::int64_t>(size), 0);
    }

    inline Result copyFile(const std::string &source, const std::string &destination) {
        File source_f, dest_f;
        if (auto rc = this->openFile(source_f, source) | this->openFile(dest_f, destination, FsOpenMode_Write); R_FAILED(rc))
            return rc;

        constexpr std::size_t buf_size = 0x100000; // 1 MiB
        auto buf = std::vector<std::uint8_t>(buf_size);

        std::size_t size = source_f.size(), offset = 0;
        while (size) {
            auto read = source_f.read(static_cast<void *>(buf.data()), buf_size, offset);
            dest_f.write(buf.data(), read, offset);
            offset += read;
            size   -= read;
        }

        source_f.close();
        dest_f.close();

        return 0;
    }

    inline FsDirEntryType getPathType(const std::string &path, Result& rc) {
        FsDirEntryType type;
        rc = fsFsGetEntryType(&this->handle, path.c_str(), &type);   
        return type;
    }

    inline bool isDirectory(const std::string &path) {
        Result rc;
        bool isDir = getPathType(path, rc) == FsDirEntryType_Dir;
        if (R_FAILED(rc))
        {
            return false;
        }
        
        return isDir;
    }

    inline bool is_file(const std::string &path) {
        Result rc;
        bool isFile = getPathType(path, rc) == FsDirEntryType_File;
        if (R_FAILED(rc))
        {
            return false;
        }        
        return isFile;
    }

    inline FsTimeStampRaw getTimestamp(const std::string &path) {
        FsTimeStampRaw ts = {};
        fsFsGetFileTimeStampRaw(&this->handle, path.c_str(), &ts);
        return ts;
    }

    inline std::uint64_t getTimestampCreated(const std::string &path) {
        return this->getTimestamp(path).created;
    }

    inline std::uint64_t getTimestampModified(const std::string &path) {
        return this->getTimestamp(path).modified;
    }

    inline Result moveDirectory(const std::string &old_path, const std::string &new_path) {
        return fsFsRenameDirectory(&this->handle, old_path.c_str(), new_path.c_str());
    }

    // original method from JKSV
    inline Result copyDirectory(const std::string &old_path, const std::string &new_path) {
        Directory old_dir;
        old_dir.open(&this->handle, old_path);
        auto list = old_dir.list();

        for(size_t i = 0; i < old_dir.count(); i++)
        {
            std::string newFrom = old_path + list[i].name;
            std::string newTo   = new_path + list[i].name;

            if(list[i].type == FsDirEntryType_Dir)
            {
                newFrom += "/";
                newTo += "/";
                mkdir(newTo.substr(0, newTo.length() - 1).c_str(), 0777);

                return copyDirectory(newFrom, newTo);
            }
            else
            {
                Result rc = copyFile(newFrom, newTo);
                if (R_SUCCEEDED(rc))
                {
                    flush();
                }
                return rc;
            }
        }  
    }

    inline Result moveFile(const std::string &old_path, const std::string &new_path) {
        return fsFsRenameFile(&this->handle, old_path.c_str(), new_path.c_str());
    }

    inline Result deleteDirectory(const std::string &path) {
        return fsFsDeleteDirectoryRecursively(&this->handle, path.c_str());
    }

    inline Result deleteFile(const std::string &path) {
        return fsFsDeleteFile(&this->handle, path.c_str());
    }
};

} // namespace fs
