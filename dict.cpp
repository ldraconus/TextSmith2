#include "5th.h"

#include <iostream>

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#ifdef QT_CORE_LIB
#include <QLoggingCategory>
#endif

namespace fifth {

  static value getWord(vm* v, exe word, stack& user) {
    word->eval(v);
    return user.pop();
  }

  // -[ I/O ]-8<------8<------8<------8<------8<------8<------8<------8<-----

  /**
   * word:  word
   * alias: get, gets
   * user:  ---> s
   *
   * Get the next word from the input string.  Honors quotes ( ''' & '"' ),
   * skips leading spaces.  Pushes an empty string onto the stack on end of
   * string.  Recognizes numbers are different from strings and words.
   **/
  static void word(vm* v) {
    str strng = v->input();
    stack& user = v->user();
    if (strng.isEmpty()) {
      user.push(exe(nullptr));
      DBG_MSG("word: <null>");
      return;
    }

    QString work;
    auto x = strng.str();
    QChar quote;
    bool quoted = false;
    int count = 0;
    int state = 0;
    for (qsizetype i = 0; i < x.size(); ++i) {
      const auto ptr = x[i];
      if (state < 0) break;
      ++count;
      switch (state) {
      case 0:
        if (!ptr.isSpace()) {
          if (QString("'\"").contains(ptr)) {
            quote = ptr;
            quoted = true;
            state = 1;
            work = "";
          } else {
              state = 3;
              work = ptr;
              break;
          }
        }
        break;
      case 1:
        if (ptr == QChar('\\')) state = 2;
        else {
          if (ptr == quote) state = -1;
          else work += ptr;
        }
        break;
      case 2:
        if (QString("nrt").contains(ptr)) {
          if (ptr == QChar('n')) work += '\n';
          else if (ptr == QChar('r')) work += '\r';
          else if (ptr == QChar('t')) work += '\t';
        }
        else work += ptr;
        state = 1;
        break;
      case 3:
        if (ptr.isSpace()) state = -1;
        else work += ptr;
        break;
      }
    }

    bool isNum = true;
    x = work;
    bool first = true;
    for (qsizetype i = 0; i < x.size(); ++i) {
      const auto wrd = x[i];
      if (first) {
        first = false;
        if (wrd == QChar('-')) continue;
      }
      if (!wrd.isDigit()) {
        isNum = false;
        break;
      }
    }

    isNum &= !quoted && !((work.size() == 1 && work[0] == '-') || work.isEmpty());

    if (isNum) {
        user.push(work.toLongLong());
        DBG_MSG("word: " + work + " <number>");
    } else {
        if (!quoted && work.isEmpty()) {
            user.push(exe(nullptr));
            DBG_MSG("word: <null>");
        } else {
            user.push(work);
            DBG_MSG("word: '" + work + "' <string>");
        }
    }

    strng = strng.substr(count);
    v->setInput(strng);
  }

  /**
   * word:  / *
   * user:  --->
   *
   * Simply eat text until * /
   **/
  static void blockComment(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];
    value x = ::FString("");
    for (; ; ) {
      x = getWord(v, word, user);
      if (x.isExe()) break;
      if (x.isStr() && x.asString() == ::FString("*/")) break;
    }
  }

  /**
   * word:  //
   * user:  --->
   *
   * Simply eat text until end-of-line ('\n')
   **/
  static void comment(vm* v) {
    str strng = v->input();
    int count = 0;
    auto x = strng.str();
    for (qsizetype i = 0; i < x.size(); ++i) {
      const auto ptr = x[i];
      ++count;
      if (QChar('\n') == ptr) break;
    }
    strng = strng.substr(count);
    v->setInput(strng);
  }

  /**
   * word:  peek
   * user:  ---> w
   *
   * Calls word to get the next word from the input stream.  It then puts the
   * word back on the input stream.  It may not duplicate what was entered
   * but what word returns next will be the same.
   */
  void peek(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];

    word->eval(v);
    auto val = user.top();
    str input = v->input();
    if (val.isStr()) {
      str wrd = val.asString();
      bool needsQuotes = false;
      for (qsizetype i = 0; i < wrd.size(); ++i) {
        const auto x = wrd[i];
        if (x == ' ' || x == '\t' || x == '\r' || x == '\n') {
          needsQuotes = true;
          break;
        }
      }

      if (!needsQuotes) {
          bool isNum = true;
          auto x = wrd.toStdString();
          const char* wrd = x.c_str();                         // NOLINT
          if (*wrd == '-') ++wrd;                              // NOLINT
          for (; *wrd; ++wrd) {                                // NOLINT
              if (!isdigit(*wrd)) {
                  isNum = false;
                  break;
              }
          }
          if (isNum) needsQuotes = true;
      }

      if (!needsQuotes) input = wrd + " " + input;
      else {
        str newWord = "'";
        for (const auto& x: wrd) {
          ::FString y = ::FString(x);
               if (y == "'") newWord += "\\'";
          else if (y == "\n") newWord += "\\n";
          else if (y == "\r") newWord += "\\r";
          else if (y == "\t") newWord += "\\t";
          else newWord += y;
        }
        newWord += "'";
        input = newWord + " " + input;
      }
    } else if (val.isNum()) input = val.asString() + " " + input;

    v->setInput(input);
  }

  /**
   * word:  put
   * alias: puts
   * user:  a --->
   *
   * Take the top item 'a' and append it as a string to the output string.
   **/
  void put(vm* v) {
    stack& user = v->user();
    auto output = v->output();
    auto a = user.pop();
    output += a.asString();
    v->setOutput(output);
  }

  /**
   * word:  flsh
   * user:  --->
   *
   * Forces outpout to stdout
   **/
  void flsh(vm* v) {
    auto output = v->output();
    std::cout << output.toStdString();
    std::cout.flush();
    v->setOutput("");
  }

  /**
   * word:  dbg
   * user:  a --->
   *
   * Debug output a
   **/
  void dbg(vm* v) {
    stack& user = v->user();
    auto output = v->output();
    auto a = user.pop();
    auto val = a.asString().str();
    DBG_VAR(a.asString().str(), v->varName(val).str());
  }

  // -[ Control ]----->8------>8------>8------>8------>8------>8------>8-----

  /**
   * word: bag
   * user: -u-> s
   *
   * Like var, but creates a local map.
   */
  void makeBag(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];
    auto a = getWord(v, word, user);
    if (!a.isStr()) {
        user.push(a);
        return;
    }
    str var = a.asString();
    v->createBag(var);
    user.push(var);
  }

  /**
   * word:  def
   * alias: func
   *
   * Compile a new word.  This is an immediate function, meaning it is executed
   * as soon as the wordis seen, iterrupting any current compile.  The current
   * word being compiled is available vie 'v->code()'.
   *
   * Here is an example of this word:
   *
   * [code]
   * def nl {
   *   "\n" puts
   * }
   * [code]
   *
   * Note that because the code block is creted locally and added to the
   * dictionary before compilation starts recursion is allowed.  Be careful
   * of infinte loops!
   **/
  void def(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];
    word->eval(v);

    auto s = getWord(v, word, user);
    if (!s.isStr()) return;
    str name = s.asString();

    s = getWord(v, word, user);
    if (!s.isStr() || s.asString() != "{") return;

    exe code = dynamic_cast<exe>(v->newCompiled());
    v->addBuiltin(name, code);
    v->compile("}", code);
  }

  /**
   * word: lookup
   * user: s ---> e
   *
   * Take the string on the top of the stack and if it's in the dictionary thens
   * put the callable word on the stack.
   **/
  void lookup(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    if (a.isStr()) {
      str s = a.asString();
      if (v->contains(s)) user.push((*v)[s]);
      else user.push(-1);
    } else user.push(-1);
  }

  /**
   * word: {
   * user: ---> e
   *
   * compile the next words in the input until reaching a '}' and leave it
   * on the stack if not compiling, call it if compiling in parent word.
   **/
  void subfunc(vm* v) {
    stack& user = v->user();
    exe code = dynamic_cast<exe>(v->newCompiled());  // NOLINT
//DBG_MSG(QString("S 0x%1").arg((long long) v->code(), 0, 16));
    v->compile("}", code);
//DBG_MSG(QString("S 0x%1").arg((long long) v->code(), 0, 16));
    if (v->compiling()) {
        if (compiled* parent = dynamic_cast<compiled*>(v->code()); parent) {
            parent->call(code);
        }
        else user.push(code);
    } else user.push(code);
  }

  /**
   * word: [
   * user: --->
   *
   * compile the next words on the stack until reaching a ']' and then
   * execute it.
   *
   **/
  void immed(vm* v) {
    auto& user = v->user();
    exe word = (*v)["word"];
    value x;
    str name;
    x = getWord(v, word, user).asString();
    name = x.asString();
    while (x.isNum() || (x.isStr() && name != "]")) {
      if (x.isNum()) user.push(x);
      else {
        if (v->contains(name)) {
          exe func = (*v)[name];
          func->eval(v);
        } else user.push(x);
      }
      x = getWord(v, word, user).asString();
      name = x.asString();
    }
  }

  /**
   * word: push
   * user: a --->
   *
   * take the top word on the stack and add a push command of it into
   * the current word being compiled.
   **/
  void push(vm* v) {
      stack& user = v->user();
      auto code = dynamic_cast<compiled*>(v->code());
      code->push(user.pop());
  }

  /**
   * word: call
   * user: e --->
   *
   * Take the top on the stack and if it's a callable word then call it,
   * otherwise just ignore it.
   **/
  void call(vm* v) {
    stack& user = v->user();
    auto e = user.pop();
    if (e.isExe()) {
      auto func = e.asCallable();
      func->eval(v);
    }
  }

  /**
   * word: \x20if
   * user: a t f --->
   *
   * Run the compiled if
   **/
  void doIf(vm* v) {
    stack& user = v->user();
    auto falseBranch = user.pop();
    auto trueBranch = user.pop();
    auto val = user.pop();
    if ((val.isNum() && val.asNumber() != 0) ||
      (val.isStr() && val.asString().size() != 0)) {
      if (trueBranch.isExe()) {
        exe call = trueBranch.asCallable();
        if (call != nullptr) call->eval(v);
      } else user.push(trueBranch);
    } else {
      if (falseBranch.isExe()) {
        exe call = falseBranch.asCallable();
        if (call != nullptr) call->eval(v);
      } else user.push(falseBranch);
    }
  }

  /**
   * word: if
   * user: a --->
   *
   * Take the top on the stack and if it's true, then execute the follwowing
   * word, and optionally execute the next world after an else if it's false.
   **/
  void ifThen(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];

    if (v->compiling()) {
      // process words until an end is seen
      //code->call((*v)[" if"]);
    } else {
      auto test = user.pop();
      if (test.asNumber()) {
          auto x = v->get(word);
          auto name = x.asString();
          while (!x.isExe()) {
            if (x.isNum()) user.push(x);
            else {
              if (v->contains(name)) {
                auto func = (*v)[name];
                func->eval(v);
              } else {
                  if (name == "else" || name == "end") break;
                  user.push(x);
              }
            }
            x = v->get(word);
            name = x.asString();
          }
          if (name == "else") {
            int ifc = 1;
            for (; !x.isExe(); (x = v->get(word)), name = x.asString()) {
              if (name == "if") ++ifc;
              if (name == "end") {
                --ifc;
                if (!ifc) break;
            }
          }
        }
      } else {
        int ifc = 1;
        str name = "not-if";
        value x;
        x = v->get(word);
        name = x.asString();
        while (!x.isExe() && ifc) {
          if (name == "if") ++ifc;
          if (name == "end") {
              --ifc;
              if (!ifc) break;
          }
          if (ifc == 1 && name == "else") break;
          x = v->get(word);
          name = x.asString();
        }
        if (name == "end") {
          return;
        }
        x = v->get(word);
        name = x.asString();
        while (!x.isExe()) {
          if (x.isNum()) user.push(x);
          else {
            if (v->contains(name)) {
              auto func = (*v)[name];
              func->eval(v);
            } else {
                if (name == "end") break;
                user.push(x);
            }
          }
          x = v->get(word);
          name = x.asString();
        }
      }
    }
  }

  /**
   * word: \x20for
   * user: s e s a --->
   *
   * Run the compiled for
   **/
  void doFor(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto r = user.pop();
    auto e = user.pop();
    auto s = user.pop();
    auto body = b.asCallable();
    auto end = e.asNumber();
    auto start = s.asNumber();
    auto var = r.asString();
    num step = 1;
    if (start > end) step = -1;
    for (num i = start; (step > 0) ? (i <= end) : (i >= end); i += step) {
        auto val = value(i);
        v->set(var, val);
        body->eval(v);
    }
  }

  /**
   * word: for
   * user: a b --->
   *
   * for <var> <body>
   **/
  void For(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];

    auto val = v->get(word);
    if (!val.isStr()) return;
    if (v->compiling()) {
      static auto func = fifth::builtin(doFor);
      auto code = dynamic_cast<fifth::compiled*>(v->code());
      code->push(val);
      v->doNextWord(word, "next", code);
      code->call(&func);
    } else {
      auto e = user.pop();
      auto s = user.pop();
      auto body = v->input().str();
      auto end = e.asNumber();
      auto start = s.asNumber();
      num by = 1;
      str var = val.asString();
//DBG_MSG("for " + var.str() + "=" + s.asString().str() + " to " + e.asString().str() + ((start > end) ? " by -1" : ""));
      if (start > end) by = -1;
      bool ranOnce = false;
      for (num i = start; (by > 0) ? (i <= end) : (i >= end); i += by) {
        auto val = value(i);
//DBG_VAR(var.str(), i);
        v->set(var, val);
        v->step(user, word, body, "next");
        ranOnce = true;
      }
      if (!ranOnce) v->skipToEnd(); // for loop did not run, test was false
    }
  }

  /**
   * Run the compiled while
   **/
  void doWhile(vm* v) {
    stack& user = v->user();
    auto body = user.pop();
    auto val = user.pop();
    num tf = val.asNumber();
    while (tf != 0) {
      if (body.isExe()) body.asCallable()->eval(v);
      else user.push(body);
      val = user.pop();
      tf = val.asNumber();
    }
  }

  /**
   * word: while
   * user: a --->
   *
   * Take the top on the stack and if it's true, loop.  Each iteration of the
   * loop check the stack top as well.
   **/
  void While(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];

    auto code = dynamic_cast<compiled*>(v->code());
    if (v->compiling()) {
        static auto func = builtin(doWhile);
        code->call(&func);
        v->doNextWord(word, "next", code);
    }
    else {
      auto body = v->input().str();
      bool ranOnce = false;
      value condition = user.pop();
      while (!condition.isExe() && (condition.asNumber() || !condition.asString().empty())) {
        v->step(user, word, body, "next");
        condition = user.pop();
        ranOnce = true;
      }
      if (!ranOnce) v->skipToEnd(); // for loop did not run, test was false
    }
  }

  void Dump(vm* v) {
      v->dump();
  }
  /**
   * word: <var
   * user: s ---> s
   *
   * Create variable in the current compile context.  Name it the top string.
   * At run time leave it on the stack.
   **/
  void dynamicVar(vm* v) {
    stack& user = v->user();
    auto a = user.top();
    if (!a.isStr()) {
        user.push(a);
        return;
    }
    str var = a.asString();
    v->create(var);
  }

  /**
   * word: var
   * user: ---> s
   *
   * Create variable in the current compile context.  Name it the next word
   * from the input stream.  At run time leave it on the stack
   **/
  void var(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];
    auto a = getWord(v, word, user);
    if (!a.isStr()) {
        user.push(a);
        return;
    }
    str var = a.asString();
    v->create(var);
    user.push(var);
  }

  /**
   * word: <--
   * user: s a --->
   *
   * Store a in previously creted variable s
   **/
  void lset(vm* v) {
    stack& user = v->user();

    auto a = user.pop();
    auto s = user.pop();
    if (!s.isStr()) return;
    str var = s.asString();
//DBG_VAL("lset " + var.str(), a);
    v->set(var, a);
  }

  /**
   * word: [<--]
   * user: s a b --->
   *
   * Store a in previously creted bag s at a
   **/
  void lsetBag(vm* v) {
    stack& user = v->user();

    auto b = user.pop();
    auto a = user.pop();
    auto s = user.pop();
    if (!s.isStr()) return;
    str bag = s.asString();
    v->set(bag, a.asString(), b);
  }

  /**
   * word: -->
   * user: a s --->
   *
   * Store a in previously creted variable s
   **/
  void rset(vm* v) {
    stack& user = v->user();

    auto s = user.pop();
    auto a = user.pop();
    if (!s.isStr()) return;
    str var = s.asString();
    v->set(var, a);
  }

  /**
   * word: [-->]
   * user: b a s --->
   *
   * Store a in previously creted bag s at a
   **/
  void rsetBag(vm* v) {
    stack& user = v->user();

    auto s = user.pop();
    auto a = user.pop();
    auto b = user.pop();
    if (!s.isStr()) return;
    str bag = s.asString();
    v->set(bag, a.asString(), b);
  }

  /**
   * word: @
   * alias: fetch
   * user: s ---> a
   *
   * Retrieve the stored value.
   **/
  void fetch(vm* v) {
    stack& user = v->user();
    auto s = user.pop();
    if (!s.isStr()) {
        user.push(s);
        return;
    }
    auto var = s.asString();
    auto val = v->get(var);
//DBG_VAL("fetch " + var.str(), val);
    user.push(val);
  }

  /**
   * word: [@]
   * alias: [fetch]
   * user: s a ---> a
   *
   * Retrieve the stored value.
   **/
  void fetchBag(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto s = user.pop();
    if (!s.isStr()) {
        user.push(s);
        return;
    }
    auto var = s.asString();
    auto val = v->get(var, a.asString());
    user.push(val);
  }

  /**
   * word: dis
   * user: ---> s
   *
   * Get next word from input and produce a disassembly onto the stack
   **/
  void dis(vm* v) {
    stack& user = v->user();
    exe word = (*v)["word"];

    word->eval(v);
    auto s = user.pop();
    if (!s.isStr()) {
        user.push(s);
        return;
    }

    str st = s.asString();
    if (v->contains(st)) {
      exe call = (*v)[st];
      auto code = dynamic_cast<compiled*>(call);
      if (code == nullptr) user.push(st + ": builtin\n");
      str disassem = code->disassemble(v);
      user.push(st + ":\n" + disassem);
    } else {
      user.push(st + ": not in dictionary\n");
    }
  }

  // -[ Math ]>8------>8------>8------>8------>8------>8------>8------>8-----

  /**
   * word: ++
   * user: s --->
   *
   * Take the top var on the stack and add 1 to it. If 's' is not a number var do nothing.
   **/
  void inc(vm* v) {
    stack& user = v->user();
    auto s = user.pop();

    if (s.isStr()) {
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() + 1;
      v->set(var, val);
    }
  }

  /**
   * word: --
   * user: s --->
   *
   * Take the top var on the stack and subtract 1 from it. If 's' is not a number var do nothing.
   **/
  void dec(vm* v) {
    stack& user = v->user();
    auto s = user.pop();

    if (s.isStr()) {
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() - 1;
      v->set(var, val);
    }
  }

  /**
   * word: =+
   * user: s a --->
   *
   * Take the var s and add a to it. If 's' is not a number var do nothing.
   **/
  void laddTo(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto s = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() + num;
      v->set(var, val);
    }
  }

  /**
   * word: +=
   * user: a s --->
   *
   * Take the var s and add a to it. If 's' is not a number var do nothing.
   **/
  void raddTo(vm* v) {
    stack& user = v->user();
    auto s = user.pop();
    auto a = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() + num;
      v->set(var, val);
    }
  }

  /**
   * word: -=
   * user: a s --->
   *
   * Take the var s and subtract a from it. If 's' is not a number var do nothing.
   **/
  void rsubTo(vm* v) {
    stack& user = v->user();
    auto s = user.pop();
    auto a = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() - num;
      v->set(var, val);
    }
  }

  /**
   * word: =-
   * user: s a --->
   *
   * Take the var s and subtract a from it. If 's' is not a number var do nothing.
   **/
  void lsubTo(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto s = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() - num;
      v->set(var, val);
    }
  }

  /**
   * word: *=
   * user: a s --->
   *
   * Take the var s and multiply it by a. If 's' is not a number var do nothing.
   **/
  void rmulTo(vm* v) {
    stack& user = v->user();
    auto s = user.pop();
    auto a = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() * num;
      v->set(var, val);
    }
  }

  /**
   * word: =*
   * user: s a --->
   *
   * Take the var s and multiply it by a. If 's' is not a number var do nothing.
   **/
  void lmulTo(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto s = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      val = val.asNumber() * num;
      v->set(var, val);
    }
  }

  /**
   * word: /=
   * user: a s --->
   *
   * Take the var s and divide it by a. If 's' is not a number var do nothing.
   **/
  void rdivTo(vm* v) {
    stack& user = v->user();
    auto s = user.pop();
    auto a = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      if (num < 0) val = -LONG_MAX;
      else val = val.asNumber() / num;
      v->set(var, val);
    }
  }

  /**
   * word: =/
   * user: s a --->
   *
   * Take the var s and divide it by a. If 's' is not a number var do nothing.
   **/
  void ldivTo(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto s = user.pop();

    if (s.isStr() && a.isNum()) {
      auto num = a.asNumber();
      auto var = s.asString();
      auto val = v->get(var);
      if (num < 0) val = -LONG_MAX;
      else val = val.asNumber() / num;
      v->set(var, val);
    }
  }

  /**
   * word: %=
   * user: a s --->
   *
   * Take the var s and modulo it by a. If 's' is not a number var do nothing.
   **/
  void rmodTo(vm* v) {
      stack& user = v->user();
      auto s = user.pop();
      auto a = user.pop();

      if (s.isStr() && a.isNum()) {
          auto num = a.asNumber();
          auto var = s.asString();
          auto val = v->get(var);
          if (num == 0) val = int(0);
          else val = val.asNumber() % num;
          v->set(var, val);
      }
  }

  /**
   * word: =^
   * user: s a --->
   *
   * Take the var s and modula it by a. If 's' is not a number var do nothing.
   **/
  void lpowerTo(vm* v) {
      stack& user = v->user();
      auto a = user.pop();
      auto s = user.pop();

      if (s.isStr() && a.isNum()) {
          auto nm = a.asNumber();
          auto var = s.asString();
          auto val = v->get(var);
          if (nm == 0) val = num(1);
          else val = num(pow(val.asNumber(), nm));
          v->set(var, val);
      }
  }

  /**
   * word: ^=
   * user: a s --->
   *
   * Take the var s and modulo it by a. If 's' is not a number var do nothing.
   **/
  void rpowerTo(vm* v) {
      stack& user = v->user();
      auto s = user.pop();
      auto a = user.pop();

      if (s.isStr() && a.isNum()) {
          auto nm = a.asNumber();
          auto var = s.asString();
          auto val = v->get(var);
          if (nm == 0) val = num(1);
          else val = num(pow(val.asNumber(), nm));
          v->set(var, val);
      }
  }

  /**
   * word: =%
   * user: s a --->
   *
   * Take the var s and modula it by a. If 's' is not a number var do nothing.
   **/
  void lmodTo(vm* v) {
      stack& user = v->user();
      auto a = user.pop();
      auto s = user.pop();

      if (s.isStr() && a.isNum()) {
          auto num = a.asNumber();
          auto var = s.asString();
          auto val = v->get(var);
          if (num == 0) val = int(0);
          else val = val.asNumber() % num;
          v->set(var, val);
      }
  }

  /**
   * word: +
   * user: a b ---> c
   *
   * Take the top two items on the stack and add them. If 'a' is a number
   * treat both as numbers.  If 'a' is a string treat both as strings.
   * For anything else, just push a -1.
   **/
  void add(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) {
      num num1 = a.asNumber();
      num num2 = b.asNumber();
      user.push(num1 + num2);
    } else if (a.isStr()) {
      str str1 = a.asString();
      str str2 = b.asString();
      user.push(str1 + str2);
    } else user.push(a);
  }

  /**
   * word: -
   * user: a b ---> c
   *
   * Take the top two numbers on the stack and subtract them. If 'a' is a number
   * treat both as numbers.  If 'a' is a string then just push an empty string,
   * and for anything else, just push a -1.
   **/
  void sub(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) {
      num num1 = a.asNumber();
      num num2 = b.asNumber();
      user.push(num1 - num2);
    } else if (a.isStr()) user.push(a);
      else user.push(a);
  }

  /**
   * word: *
   * user: a b ---> c
   *
   * Take the top two things on the stack and multiply them. If 'a' is a number
   * treat both as numbers.  If 'a' is a string then duplicate it 'b' times,
   * and for anything else, just push a -1.
   **/
  void mult(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) {
      num num1 = a.asNumber();
      num num2 = b.asNumber();
      user.push(num1 * num2);
    } else if (a.isStr()) {
      str str1 = a.asString();
      num num1 = b.asNumber();
      str str2;
      for (num i = 0; i < num1; ++i) str2 += str1;
      user.push(str2);
    } else user.push(a);
  }

  /**
   * word: /
   * user: a b ---> c
   *     : s t ---> u... c
   *
   * Take the top two things on the stack and divide them. If 'a' is a number
   * treat both as numbers.  If both are a strings then split 's' into parts
   * by 't'.  Push the parts onto the stack (last on top of stack) and the number
   * on top of that.
   **/
  void div(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();
//DBG_VAL("a", a);
//DBG_VAL("b", b);

    if (a.isNum()) {
      num num1 = a.asNumber();
      num num2 = b.asNumber();
      if (num2 == 0) {
        if (num1 < 0) user.push(-LONG_MAX);
        else user.push(LONG_MAX);
      } else user.push(num1 / num2);
    } else if (a.isStr()) {
      str str1 = a.asString();
      if (b.isStr()) {
        num count = 0;
        str str2 = b.asString();
        num pos = -1;
        while ((pos = str1.indexOf(str2)) != -1) {
            user.push(str1.left(pos));
            ++count;
            str1 = str1.substr(pos + str2.size());
        }
        user.push(str1);
        ++count;
        user.push(count);
      } else user.push(num(0));
    } else user.push(a);
  }

  /**
   * word: %
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and modulus them.  Strings as 'a', push
   * a "", or anything else -1.
   **/
  void mod(vm* v) {
      stack& user = v->user();
      auto b = user.pop();
      auto a = user.pop();

      if (a.isNum()) {
          num num1 = a.asNumber();
          num num2 = b.asNumber();
          if (num2 == 0) user.push(num(0));
          else {
              num num3 = num1 % num2;
              user.push(num3);
          }
      } else if (a.isStr()) user.push("");
      else user.push(a);
  }

  /**
   * word: ^
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and rais a by b them.  Strings as 'a', push
   * a "", or anything else 0.
   **/
  void power(vm* v) {
      stack& user = v->user();
      auto b = user.pop();
      auto a = user.pop();

      if (a.isNum()) {
          num num1 = a.asNumber();
          num num2 = b.asNumber();
          if (num2 == 0) user.push(num(1));
          else {
              num num3 = num(pow(num1, num2));
              user.push(num3);
          }
      } else if (a.isStr()) user.push("");
      else user.push(0);
  }

  /**
   * word: <
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if less than, a 0 otherwise.
   **/
  void less(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() < b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() < b.asString()));
    else user.push(num(0));
  }

  /**
   * word: <=
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if less than or equal, a 0 otherwise.
   **/
  void lessEq(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() <= b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() <= b.asString()));
    else user.push(num(0));
  }

  /**
   * word: <>
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if not equal, a 0 otherwise.
   **/
  void notEq(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() != b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() != b.asString()));
    else user.push(num(0));
  }

  /**
   * word: =
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if equal, a 0 otherwise.
   **/
  void equal(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() == b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() == b.asString()));
    else user.push(num(0));
  }

  /**
   * word: >=
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if greater or equal, a 0 otherwise.
   **/
  void greatEq(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() >= b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() >= b.asString()));
    else user.push(num(0));
  }

  /**
   * word: >
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Whatever 'a' is compare
   * them as that type. Push a 1 if greater, a 0 otherwise.
   **/
  void greater(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    if (a.isNum()) user.push(num(a.asNumber() > b.asNumber()));
    else if (a.isStr()) user.push(num(a.asString() > b.asString()));
    else user.push(num(0));
  }

  /**
   * word: and
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Both are compared
   * as numbers. Push a 1 if both non-zero, a 0 otherwise.
   **/
  void And(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    user.push(num(a.asNumber() && b.asNumber()));
  }

  /**
   * word: or
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Both are compared
   * as numbers. Push a 1 if at least one is non-zero, a 0 otherwise.
   **/
  void Or(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    user.push(num(a.asNumber() || b.asNumber()));
  }

  /**
   * word: xor
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Both are compared
   * as numbers. Push a 1 if only one is non-zero, a 0 otherwise.
   **/
  void Xor(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    num num1 = a.asNumber();
    num num2 = b.asNumber();

    user.push(num(!(num1 && num2) && (num1 || num2)));
  }

  /**
   * word: nand
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Both are compared
   * as numbers. Push a 1 if one or both are zero, a 0 otherwise.
   **/
  void Nand(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    user.push(num(!(a.asNumber() && b.asNumber())));
  }

  /**
   * word: nor
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Both are compared
   * as numbers. Push a 1 if both are zero, a 0 otherwise.
   **/
  void Nor(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();

    user.push(num(!(a.asNumber() || b.asNumber())));
  }

  /**
   * word: not
   * user: a ---> c
   *
   * Take 'a' off the stack, and test it.  It is tested as a number. Push a 1
   * if it is a zero, a 0 otherwise.
   **/
  void Not(vm* v) {
    stack& user = v->user();
    auto a = user.pop();

    user.push(num(!(a.asNumber())));
  }

  /**
   * word: neg
   * user: a ---> c
   *
   * Take 'a' off the stack, and negate it. It is treated as a number.
   **/
  void neg(vm* v) {
    stack& user = v->user();
    auto a = user.pop();

    user.push(num(-(a.asNumber())));
  }

  /**
   * word: prec
   * user: s a --->
   *
   * Sets the precedence for an operator.  As a side effect, the operator is now
   * expected to take two items from the stack and put one after it's done.  If
   * it doesn't do this, the results are ... unpredictable.
   **/
  void prec(vm* v) {
    stack& user = v->user();
    auto& p = v->precTable();
    auto a = user.pop();
    auto s = user.pop();

    if (!s.isStr() || a.isNum()) return;

    str op = s.asString();
    num pr = a.asNumber();
    p.add(op, int(pr));
  }

  static void doRef(vm* v, compiled* code, value vl) {
    if (vl.isExe()) return;
    code->push(vl);
    if (vl.isStr()) {
      str vs = vl.asString();
      if (v->get(vs).asNumber() != -1) {
        exe fetch = (*v)["@"];
        code->call(fetch);
      }
    }
  }

  static void doOp(vm* v, precedence& prec, compiled* code,
                   const str& op, stack& val) {
    static callable dummy;

    exe f = (*v)[op];
    if (prec[op] != 0) {
      auto vl2 = val.pop();
      auto vl1 = val.pop();
      doRef(v, code, vl1);
      doRef(v, code, vl2);
      code->call(f);
      val.push(&dummy);
    } else {
      auto vl = val.pop();
      if (f != nullptr) {
        doRef(v, code, vl);
        code->call(f);
        val.push(&dummy);
      }
    }
  }

  /**
   * word: (
   * user: --->
   *
   * Read input word by word and compile it using algebraic rules.  Most words
   * not found in the precedence table will be treating like not.  If it is in
   * the table, it's expected to take two items from the stack and leave one.
   * if the word is a variable, it is fetched at the correct time.
   **/
  void algebra(vm* v) {
    stack& user = v->user();
    auto code = dynamic_cast<compiled*>(v->code());
    exe word = (*v)["word"];

    stack op;
    stack val;
    auto& prec = v->precTable();

    op.push(")");
    while (!op.empty()) {
      word->eval(v);
      auto x = user.pop();
      if (x.asString().isEmpty()) break;
      if (x.isNum()) val.push(x);
      else {
        if (x.isStr()) {
          if (v->isVar(x.asString())) val.push(x);
          else {
            str curr = x.asString();
            if (curr == "(") {
              op.push(")");
            } else if (curr == ")") {
              str st;
              do {                                                  // NOLINT
                st = op.pop().asString();
                if (st != ")") doOp(v, prec, code, st, val);
              } while (st != ")");
            } else {
              if (!v->contains(curr)) val.push(curr);
              else {
                exe w = (*v)[curr];
                if (w->isImmediate()) {
                  w->eval(v);
                  val.push(user.pop());
                } else {
                  while (!op.empty() &&
                         op.top().asString() != ")" &&
                         prec[op.top().asString()] < prec[curr]) {
                    str st = op.pop().asString();
                    doOp(v, prec, code, st, val);
                  }
                  op.push(x);
                }
              }
            }
          }
        }
      }
    }
    while (!op.empty() &&
           op.top().asString() != ")") {
      str st = op.pop().asString();
      doOp(v, prec, code, st, val);
    }
  }

  /**
   * word: min
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Push the smaller of the two
   * onto the stack.
   **/
  void Min(vm* v) {
      stack& user = v->user();
      auto b = user.pop();
      auto a = user.pop();

      user.push(a < b ? a : b);
  }

  /**
   * word: max
   * user: a b ---> c
   *
   * Take 'a' and 'b' off the stack, and compare them.  Push the larger of the two
   * onto the stack.
   **/
  void Max(vm* v) {
      stack& user = v->user();
      auto b = user.pop();
      auto a = user.pop();

      user.push(a < b ? b : a);
  }

  // -[ String ]------>8------>8------>8------>8------>8------>8------>8-----

  /**
   * word: join
   * user: s... t a ---> u
   *
   * Take 'a' and 't' off the stack, join the top 'a' remaining items on the
   * stack together separated by 't' (this is the inverse of dividing two
   * strings).
   **/
  void join(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    auto t = user.pop();

    num count = a.asNumber();
    str sep = t.asString();
    str result;
    for (num i = 0; i < count; ++i) {
      if (i != 0) result = sep + result;
      auto s = user.pop();
      str part = s.asString();
      result = part + result;
    }
    user.push(result);
  }

  /**
   * word: substr
   * user: s a b ---> t
   *
   *
   * Extract a part of a string, starting at a and extending b characters.  If
   * b is -1, assume to end-of-string.
   **/
  void substr(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();
    auto s = user.pop();

    num len = b.asNumber();
    num start = a.asNumber();
    str word = s.asString();

    if (len == 0 || start < 0 || word == "") user.push("");
    else {
      str sub;
      if (len == -1) sub = word.substr(start);
      else sub = word.substr(start, len);
      user.push(sub);
    }
  }

  /**
   * word: len
   * user: s ---> a
   *
   * Determine the length of the string on top of the stack and push that onto
   * the stack
   **/
  void len(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    if (a.isStr()) {
      str str1 = a.asString();
      user.push(num(str1.size()));
    } else user.push(a);
  }

  /**
   * word: empty?
   * user: s ---> 0|1
   *
   * 1 if a string and 0 length, 0 otherwise
   **/
  void empty(vm* v) {
      stack& user = v->user();
      auto a = user.pop();
      if (a.isStr()) {
          str str = a.asString();
          user.push(num(str.isEmpty() ? 1 : 0));
      } else user.push(num(0));
  }


  // -[ Stack ]8------>8------>8------>8------>8------>8------>8------>8-----

  /**
   * word: pop
   * user: a --->
   *
   * Take 'a' and remove it from the stack.
   **/
  void pop(vm* v) {
    stack& user = v->user();
    user.pop();
  }

  /**
   * word: over
   * user: a b ---> a b a
   *
   * Take 'a' and push it on top of 'b'.
   **/
  void over(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();
    user.push(a);
    user.push(b);
    user.push(a);
  }

  /**
   * word: swap
   * user: a b ---> b a
   *
   * Take 'a' and 'b' push 'b' and then 'a'.
   **/
  void Swap(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto a = user.pop();
    user.push(b);
    user.push(a);
  }

  /**
   * word: dup
   * user: a ---> a a
   *
   * Take 'a' and push 'a' and then 'a'.
   **/
  void dup(vm* v) {
    stack& user = v->user();
    auto a = user.pop();
    user.push(a);
    user.push(a);
  }

  /**
   * word: lrot
   * user: a b c --> c a b
   *
   * Rotate the top three items 1 to the left
   **/
  void lrot(vm* v) {
    stack& user = v->user();
    auto c = user.pop();
    auto b = user.pop();
    auto a = user.pop();
    user.push(c);
    user.push(a);
    user.push(b);
  }

  /**
   * word: rot
   * user: a b c --> b c a
   *
   * Rotate the top three items 1 to the right
   **/
  void rot(vm* v) {
    stack& user = v->user();
    auto c = user.pop();
    auto b = user.pop();
    auto a = user.pop();
    user.push(b);
    user.push(c);
    user.push(a);
  }

  /**
   * word: rev
   * user: a... b ---> ...a
   *
   * Take the top 'b' items 'a' and reverse the order on the stack
   **/
  void rev(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    if (!b.isNum()) {
        user.push(0);
        return;
    }
    num num1 = b.asNumber();
    if (user.size() < size_t(num1)) {
        user.push(0);
        return;
    }
    num j = num1 - 1;
    for (num i = 0; i < num1 / 2; ++i, --j) {
      auto t = user[i];
      user[i] = user[j];
      user[j] = t;
    }
  }

  /**
   * word: nth
   * user: a... b ---> a...a
   *
   * Copy b-th item to top of stack
   **/
  void nth(vm* v) {
    stack& user = v->user();
    auto b = user.pop();
    auto num1 = b.asNumber();
    if (size_t(num1) > user.size() || num1 < 1) user.push(num(0));
    else user.push(user[num1]);
  }

  // ->8------>8------>8------>8------>8------>8------>8------>8------>8-----

  constexpr int POWER_PREC = 40;
  constexpr int MULT_PREC  = 40;
  constexpr int ADD_PREC   = 50;
  constexpr int COMP_PREC  = 60;
  constexpr int BIN_PREC   = 70;

  void vm::init() {
    // --[ I/O ]--------------
    addBuiltin("/*",     blockComment); //   -u->       Eat text until */
    addBuiltin("//",     comment);      //   -u->       Eat text until end-of-line
    addBuiltin("get",    word);         //   -u-> w     Push word to stack
    addBuiltin("gets",   word);         //   -u-> w     Push word to stack
    addBuiltin("word",   word);         //   -u-> w     Push word to stack
    addBuiltin("peek",   peek);         //   -u-> w     Push word to stack, leave
    addBuiltin("put",    put);          // a -u->       Outputs a
    addBuiltin("puts",   put);          // a -u->       Outputs a
    addBuiltin("flush",  flsh);         //   -u->       Forces output to stdout
    addBuiltin("[debug]", dbg);         // a -u->       Debug output var a and value
    addBuiltin("dump",   Dump);         //   -u->       Dump all 5th data

    // --[ Control ] ---------
    addImmediate("array",     makeBag);   //     -u-> s
    addImmediate("bag",       makeBag);   //     -u-> s
    addImmediate("struct",    makeBag);   //     -u-> s
    addImmediate("def",       def);       //     -u->
    addImmediate("func",      def);       //     -u->
    addImmediate("{",         subfunc);   //     -u-> e
    addImmediate("[",         immed);     //     -u->
    addImmediate("push",      push);      //   a -u->
    addImmediate("if",        ifThen);    //   a -u->
    addImmediate("for",       For);       // a b -u->
    addImmediate("while",     While);     //   a -u->
    addImmediate("var",       var);       //     -u-> s
    addImmediate("disassem",  dis);       //     -u-> s

    addBuiltin("<var",    dynamicVar); //    -u-> s
    addBuiltin("lookup",  lookup);   //     s -u-> e
    addBuiltin("call",    call);     //     e -u->
    addBuiltin("<--",     lset);     //   s a -u->
    addBuiltin("-->",     rset);     //   a s -u->
    addBuiltin("@",       fetch);    //     s -u-> a
    addBuiltin("fetch",   fetch);    //     s -u-> a
    addBuiltin("[<--]",   lsetBag);  // s a b -u->
    addBuiltin("[<]",     lsetBag);  // s a b -u->
    addBuiltin("[-->]",   rsetBag);  // b a s -u->
    addBuiltin("[>]",     rsetBag);  // b a s -u->
    addBuiltin("[@]",     fetchBag); //   s a -u-> a
    addBuiltin("[fetch]", fetchBag); //   s a -u-> a

    // --[ Math ]-------------
    addBuiltin("++", inc);     //     s -u->       Increment variable
    addBuiltin("--", dec);     //     s -u->       Decrement variable
    addBuiltin("=+", laddTo);  //   s a -u->       Add a to var s
    addBuiltin("+=", raddTo);  //   a s -u->       Add a to var s
    addBuiltin("=-", lsubTo);  //   s a -u->       Subtract a from var s
    addBuiltin("-=", rsubTo);  //   a s -u->       Subtract a from var s
    addBuiltin("=*", lmulTo);  //   s a -u->       Multiply var s by a
    addBuiltin("*=", rmulTo);  //   a s -u->       Multiply var s by a
    addBuiltin("=/", ldivTo);  //   s a -u->       Divide var s by a
    addBuiltin("/=", rdivTo);  //   a s -u->       Divide var s by a
    addBuiltin("=%", lmodTo);  //   s a -u->       Raise var s by a
    addBuiltin("^=", rpowerTo); //  a s -u->       Raise var s by a
    addBuiltin("=^", lpowerTo); //  s a -u->       Mod var s by a
    addBuiltin("%=", rmodTo);  //   a s -u->       Mod var s by a
    addBuiltin("+",  add);     //   a b -u-> c     Add top two values
    addBuiltin("-",  sub);     //   a b -u-> c     Subtract top two values
    addBuiltin("*",  mult);    //   a b -u-> c     Multiply top two values
    addBuiltin("/",  div);     //   a b -u-> c     Divide top two values
                               //   s t -u-> u.. a Divide top two strings
    addBuiltin("%",  mod);     //   a b -u-> c     Modulus top two values
    addBuiltin("^",  power);   //   a b -u-> c     Raise a to the power of b
    addBuiltin("<",  less);    //   a b -u-> c     Compare top two values
    addBuiltin("<=", lessEq);  //   a b -u-> c     Compare top two values
    addBuiltin("<>", notEq);   //   a b -u-> c     Compare top two values
    addBuiltin("=",  equal);   //   a b -u-> c     Compare top two values
    addBuiltin(">=", greatEq); //   a b -u-> c     Compare top two values
    addBuiltin(">",  greater); //   a b -u-> c     Compare top two values

    addBuiltin("and",  And);   //   a b -u-> c     Compare top two values
    addBuiltin("or",   Or);    //   a b -u-> c     Compare top two values
    addBuiltin("xor",  Xor);   //   a b -u-> c     Compare top two values
    addBuiltin("nand", Nand);  //   a b -u-> c     Compare top two values
    addBuiltin("xor",  Nor);   //   a b -u-> c     Compare top two values

    addBuiltin("not", Not);    //     a -u-> c     Invert top value
    addBuiltin("neg", neg);    //     a -u-> c     Negate top value

    addBuiltin("prec", prec);  //   s a -u->       Set op precedence

    addImmediate("(", algebra);  //     -u->       Compile using algebra rules

    // --[ String ]-----------
    addBuiltin("join", join);     // s.. t a -u-> u   Join 'a' strings (s) together seperated by t
    addBuiltin("substr", substr); //   s a b -u-> t   Extract part of a string
    addBuiltin("len", len);       //       s -u-> a   Push length of the string
    addBuiltin("empty?", empty);  //       a -u0> 0|1 Test a string for zero length

    // --[ Stack ]------------
    addBuiltin("pop",  pop);   //     a -u->       Discard top
    addBuiltin("swap", Swap);  //   a b -u-> b a   Swap top two
    addBuiltin("over", over);  //   a b -u-> a b a Copy 2nd
    addBuiltin("dup",  dup);   //     a -u-> a a   Duplicate top
    addBuiltin("rot",  rot);   // a b c -u-> c a b Rotate the top 3
    addBuiltin("lrot", lrot);  // a b c -u-> b c a Left rotate the top 3
    addBuiltin("rev",  rev);   // a.. b -u-> ..a   Reverse b items
    addBuiltin("nth",  nth);   // a.. b -u-> a..a  Copy b-th item to top

    addEnd("for");
    addEnd("while");

    mPrec.add("^",    POWER_PREC);
    mPrec.add("*",    MULT_PREC);
    mPrec.add("/",    MULT_PREC);
    mPrec.add("%",    MULT_PREC);
    mPrec.add("+",    ADD_PREC);
    mPrec.add("-",    ADD_PREC);
    mPrec.add("<",    COMP_PREC);
    mPrec.add("<=",   COMP_PREC);
    mPrec.add("<>",   COMP_PREC);
    mPrec.add("=",    COMP_PREC);
    mPrec.add(">=",   COMP_PREC);
    mPrec.add(">",    COMP_PREC);
    mPrec.add("and",  BIN_PREC);
    mPrec.add("or",   BIN_PREC);
    mPrec.add("xor",  BIN_PREC);
    mPrec.add("nand", BIN_PREC);
    mPrec.add("xor",  BIN_PREC);
  }
}
