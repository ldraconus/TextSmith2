#include "5th.h"

namespace fifth {

  str instruction::disassemble(vm* v) {
    switch (mPayload.index()) {
    case VALUE: return "push " + std::get<VALUE>(mPayload).asString();
    case INT:   return "indirect";
    case EXE:
        if (v->contains(std::get<EXE>(mPayload))) return "call " + (*v)[std::get<EXE>(mPayload)];
        return "call <sub-func>";
    }
    return "";
  }

  void instruction::eval(vm* v) const {
    switch (mPayload.index()) {
    case VALUE: v->user().push(std::get<VALUE>(mPayload)); break;
    case EXE:   std::get<EXE>(mPayload)->eval(v);          break;
    case INT: {
        stack& user = v->user();
        auto val = user.pop();
        if (!val.isExe()) return;
        auto exe = val.asCallable();
        if (!exe) return;
        exe->eval(v);
      }
      break;
    }
  }

  str compiled::disassemble(vm* v) {
    auto sz = mCode.size();
    str test = ::FString::number((long long)(sz));
    auto len = test.size();
    str block;
    try {
      for (size_t i = 0; i < sz; ++i) {
        auto instr = mCode[i];
        str num = ::FString::number((long long)(i));
        str line = "";
        for (auto j = num.size(); j < len; ++j) line += " ";
        line += num + " " + instr.disassemble(v) + "\n";
        block += line;
      }
    } catch (...) { block = "disassemble crash:\n" + block; }
    return block;
  }

  void compiled::eval(vm* v) const {
    for (const auto& instr: mCode) {
        instr.eval(v);
    }
  }

  value stack::operator[](size_t x) {
    if (x >= mStack.size()) return num(0);
    return mStack[mStack.size() - x];
  }

  void stack::dump() {
DBG_MSG(QString("----<User Stack>----------------------------"));
      for (size_t i = 0; i < mStack.size(); ++i) DBG_MSG("> " + mStack[i].asString().str());
  }

  value stack::top() {
    if (mStack.empty()) return num(0);
    auto val = mStack.back();
    return val;
  }

  value stack::pop() {
    if (mStack.empty()) return num(0);
    auto vl = mStack.back();
    mStack.pop_back();
    return vl;
  }

  vm::vm()
      : mCode(nullptr)
      , mCompiling(false) {
    init();
  }

  void vm::addBuiltin(const str &n, const function &f) {
    mBuiltin[n] = dynamic_cast<exe>(new builtin(f));
    mReverse[mBuiltin[n]] = n;
  }

  void vm::addBuiltin(const str& n, exe e) {
    mBuiltin[n] = e;
    mReverse[mBuiltin[n]] = n;
  }

  void vm::addImmediate(const str &n, const function &f){
    addBuiltin(n, f);
    mBuiltin[n]->setImmediate();
  }

  void vm::addImmediate(const str& n, exe e) {
    addBuiltin(n, e);
    mBuiltin[n]->setImmediate();
  }

  value vm::get(exe word) {
    word->eval(this);
    auto val = mUser.pop();
    return val;
  }

  bool vm::isVar(const str& n) {
    if (mVars.find(n) != mVars.end()) return true;
    else return false;
  }

  void vm::create(const str& n) {
    mVars[n] = nullptr;
  }

  void vm::createBag(const str& n) {
      mBags[n] = { };
  }

  value vm::get(const str& n) {
    if (mVars.find(n) != mVars.end()) return mVars[n];
    else return value("");
  }

  value vm::get(const str& n, const str& idx) {
      if (mBags.find(n) != mBags.end()) return mBags[n][idx];
      else return value("");
  }

  void vm::set(const str &n, const value &v)
  {
      if (mVars.find(n) == mVars.end())
          create(n);
      mVars[n] = v;
  }

  void vm::set(const str& n, const str& idx, const value& v) {
    if (mBags.find(n) != mBags.end()) {
      if (mBags[n].find(idx) == mBags[n].end()) mBags[n][idx] = nullptr;
      mBags[n][idx] = v;
    }
  }

  void vm::step(stack& user, exe& word, const QString& body, const QString& end) {
      setInput(body);
      fifth::value x = get(word);
      fifth::str name = x.asString();
      while (x.isNum() || name != end) {
          if (x.isNum()) user.push(x);
          else {
              if (contains(name)) {
                  auto func = (*this)[name];
                  func->eval(this);
              } else user.push(x);
          }
          x = get(word);
          name = x.asString();
      }
  }

  void vm::skipToEnd() {
    int forc = 1;
    fifth::value x;
    str name;
    exe word = mBuiltin["word"];
//str skipped;
    x = get(word);
    name = x.asString();
    while (x.isNum() || forc) {
      if (mEnds.find(name) != mEnds.end()) ++forc;
      if (name == "next") --forc;
      if (forc) {
          x = get(word);
          name = x.asString();
      }
//skipped += " " + name;
    }
//if (skipped[0] == ' ') skipped = skipped.substr(1);
//DBG_VAR("skipped", skipped.str());
  }

  exe vm::compile(const str& end, exe cd, bool) {
    ++mCompiling;
    auto save = code();

    for (exe word = mBuiltin["word"]; ; ) {
        if (!doNextWord(word, end, cd)) break;
    }

    setCode(save);

    --mCompiling;
    return cd;
  }

  void vm::disassemble(exe call) {
    compiled* code = dynamic_cast<compiled*>(call);
    auto result = code->disassemble(this);
    const auto lines = result.str().split("\n");
    for (const auto& s: lines) DBG_MSG(s.toStdString().c_str());  // NOLINT
  }

  bool vm::doNextWord(exe word, const str& end, exe cd, bool) {
    value x = get(word);
    compiled* code = dynamic_cast<compiled*>(cd);
//DBG_MSG(QString("D 0x%1").arg((long long) code, 0, 16));

    if (x.isExe()) return false;
    if (x.isStr() && x.asString() == end) return false;

    if (x.isNum()) code->push(x);
    else {
        str name = x.asString();
        if (contains(name)) {
            exe func = mBuiltin[name];
            if (func->isImmediate()) func->eval(this);
            else code->call(func);
        } else code->push(x);
    }

    return true;
  }

  void vm::dump() {
    DBG_MSG(QString("----<Dictionary>----------------------------"));
    for (const auto& a: mBuiltin) {
      DBG_MSG(a.first.str());
    }
    DBG_MSG(QString("----<Globals>-------------------------------"));
    for (const auto& g: mVars) {
        DBG_VAL(g.first.str(), g.second);
    }
    DBG_MSG(QString("----<Bags>----------------------------------"));
    for (const auto& b: mBags) {
        DBG_MSG(b.first.str() + "=" + QString("%1").arg(b.second.size()));
        for (const auto& v: b.second) {
            DBG_MSG("    " + v.first.str() + ":" + v.second.asString().str());
        }
    }
    mUser.dump();
  }

  void vm::exec(exe c) {
    exe s = mCode;
    mCode = c;
    c->eval(this);
    mCode = s;
  }

  void vm::evaluate(const str& code) {
    auto save = input();
    setInput(code);
    exe word = mBuiltin["word"];
    for (value x = get(word); !x.isExe(); x = get(word)) {
      if (x.isNum()) mUser.push(x);
      else {
        str name = x.asString();
        if (contains(name)) {
          exe func = mBuiltin[name];
          func->eval(this);
        } else mUser.push(x);
      }
    }
    setInput(save);
  }
}
