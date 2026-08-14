#pragma once

#include "zip.h"
#include <QList>
#include <QString>

#ifdef unix
#define STRERROR() strerror_r(sys, buffer, 1024)
#else
#define STRERROR() strerror_s(buffer, 1024, sys)
#endif

class Zip {
public:
    class Error {
    public:
        Error() { }
        explicit Error(const zip_error& err)
            : mErr(err) { }
        explicit Error(zip_error* err)
            : Error(*err) { }
        Error(int code, int sys = 0, const char* msg = nullptr) {
            zip_error_init(&mErr);
            static char buffer[1024];
            if (sys != 0 && msg == nullptr) { STRERROR(); mErr.str = buffer; }
            else mErr.str = const_cast<char*>(msg);
            mErr.sys_err = sys;
            mErr.zip_err = code;
        }
        int          code()    { return mErr.zip_err; }
        QString      message() { return mErr.str ? mErr.str : zip_error_strerror(&mErr); }
        int          sys_err() { return mErr.sys_err; }
        zip_error_t& error()   { return mErr; }

    private:
        zip_error_t mErr;
    };

    enum Modes {
        Create    = ZIP_CREATE,
        Exclusive = ZIP_EXCL,
        Check     = ZIP_CHECKCONS,
        Truncate  = ZIP_TRUNCATE,
        ReadOnly  = ZIP_RDONLY
    };

    enum Flags {
        Unchanged           = 0,
        CaseInsensitive     = ZIP_FL_NOCASE,
        IgnoreDorectory     = ZIP_FL_NODIR,
        ReadCompressed      = ZIP_FL_COMPRESSED,
        UseUnchanged        = ZIP_FL_UNCHANGED,
        ReadEncrypted       = ZIP_FL_ENCRYPTED,
        GuessEncoding       = ZIP_FL_ENC_GUESS,
        UseUnmodifiedString = ZIP_FL_ENC_RAW,
        FollowSepcification = ZIP_FL_ENC_STRICT,
        InLocalHeader       = ZIP_FL_LOCAL,
        InCentralDireectory = ZIP_FL_CENTRAL,
        Utf8Encoding        = ZIP_FL_ENC_UTF_8,
        Cp437Encoding       = ZIP_FL_ENC_CP437,
        Overwrite           = ZIP_FL_OVERWRITE
    };

    enum Compression {
        Default       = ZIP_CM_DEFAULT,          /* better of deflate or store */
        Store         = ZIP_CM_STORE,            /* stored (uncompressed) */
        Shrink        = ZIP_CM_SHRINK,           /* shrunk */
        Reduce1       = ZIP_CM_REDUCE_1,         /* reduced with factor 1 */
        Reduce2       = ZIP_CM_REDUCE_2,         /* reduced with factor 2 */
        Reduce3       = ZIP_CM_REDUCE_3,         /* reduced with factor 3 */
        Reduce4       = ZIP_CM_REDUCE_4,         /* reduced with factor 4 */
        Implode       = ZIP_CM_IMPLODE,          /* imploded */
        Deflate       = ZIP_CM_DEFLATE,          /* deflated */
        Deflate64     = ZIP_CM_DEFLATE64,        /* deflate64 */
        PKWareImplode = ZIP_CM_PKWARE_IMPLODE,   /* PKWARE imploding */
        BZip          = ZIP_CM_BZIP2,            /* compressed using BZIP2 algorithm */
        LZMA          = ZIP_CM_LZMA,             /* LZMA (EFS) */
        Terse         = ZIP_CM_TERSE,            /* compressed using IBM TERSE (new) */
        LZ77          = ZIP_CM_LZ77,             /* IBM LZ77 z Architecture (PFS) */
        LZMA2         = ZIP_CM_LZMA2,
        ZStd          = ZIP_CM_ZSTD,             /* Zstandard compressed data */
        XZ            = ZIP_CM_XZ,               /* XZ compressed data */
        JPEG          = ZIP_CM_JPEG,             /* Compressed Jpeg data */
        WAVPack       = ZIP_CM_WAVPACK,          /* WavPack compressed data */
        PPMD          = ZIP_CM_PPMD             /* PPMd version I, Rev 1 */

    };

    class File {
    public:
        File(Zip& z, uint64_t idx, uint64_t f)
            : mFile(zip_fopen_index(z.zip(), idx, f)) { }
        ~File() { zip_fclose(mFile); mFile = nullptr; }

        bool    invalid()                             { return mFile == nullptr; }
        int64_t read(QByteArray& data, uint64_t size) { return zip_fread(mFile, data.data(), size); }
        bool    valid()                               { return !invalid(); }

    private:
        zip_file_t* mFile { nullptr };
    };

    static constexpr auto Free = true;
    static constexpr auto Keep = false;

    class Source {
    public:
        Source(Zip& z, const QByteArray& data, bool freeMe)  { mSource = zip_source_buffer(z.zip(), data.data(), data.length(), freeMe); }
        ~Source() { zip_source_free(mSource); }

        zip_source_t* source() { return mSource; }

        bool invalid() { return mSource == nullptr; }
        bool valid()   { return !invalid(); }

    private:
        zip_source_t* mSource { nullptr };
    };

    class Stat {
    public:
        Stat(Zip& z, uint64_t idx, uint64_t f)
            : mInvalid(zip_stat_index(z.zip(), idx, f, &mStat) != 0) { }
        Stat(Zip& z, const QString& name, uint64_t f)
            : mInvalid(zip_stat(z.zip(), name.toStdString().c_str(), f, &mStat) != 0) { }

        bool     invalid() { return mInvalid; }
        uint64_t size()    { return mStat.size; }
        bool     valid()   { return !invalid(); }

    private:
        bool       mInvalid = true;
        zip_stat_t mStat;
    };

    Zip() { }
    Zip(const QString& name, const uint64_t mode = ReadOnly, int* err = nullptr) {
        mZip = zip_open(name.toStdString().c_str(), mode, err);
        if (mZip) mInvalid = false;
    }
    ~Zip() { if (mZip) zip_close(mZip); mZip = nullptr; mInvalid = true; }

    bool    addDirectory(const QString& s, uint64_t f = GuessEncoding)  { int idx = -1; return mZip ? ((idx = zip_dir_add(mZip, s.toStdString().c_str(), f)) < 0 ? idx : isError()) : isError(); }
    Source  buffer(const QByteArray& d, bool f)                         { return Source(*this, d, f ? 1 : 0); }
    Error   error()                                                     { return mError; }
    int64_t fileAdd(const QString& name, Source& source, uint64_t f)    { return zip_file_add(mZip, name.toStdString().c_str(), source.source(), f); }
    File    fileOpenIndex(uint64_t idx, uint64_t f)                     { return File(*this, idx, f); }
    bool    invalid()                                                   { return mInvalid; }
    int     fileIndex(const QString& s, uint64_t f = GuessEncoding)     { return zip_name_locate(mZip, s.toStdString().c_str(), f); }
    int     setFileCompression(uint64_t x, int32_t m, uint32_t lvl)     { return zip_set_file_compression(mZip, x, m, lvl); }
    Stat    stat(uint64_t idx, uint64_t f = GuessEncoding)              { return Stat(*this, idx, f); }
    Stat    statFileIndex(const QString& n, uint64_t f = GuessEncoding) { return Stat(*this, n, f); }
    bool    valid()                                                     { return !invalid(); }
    zip_t*  zip()                                                       { return mZip; }

private:
    zip_t* mZip = nullptr;
    bool   mInvalid = true;
    Error  mError;

    bool isError()                               { if (mZip) mError = Error(zip_get_error(mZip)); else mError = Error(ZIP_ER_ZIPCLOSED, 0, "No zip file currently open"); return false; }
    bool isError(int e, int s)                   { mError = Error(e, s); return false; }
    bool isError(int e, const char* m = nullptr) { mError = Error(e, 0, m); return false; }
};

#undef CSTR
#undef BLK
