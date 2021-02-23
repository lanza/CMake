#pragma once

#include <iostream>

#ifdef WIN32
extern "C" {
#include <editline/readline.h>
}
#include <stdlib.h>
class Editline {
public:
  Editline() {}

  std::string GetLine() {
    char *line;
    line = readline("(cmdb) ");
    std::string l = line;
    char c;
    int i = 0;
    while ((c = line[i]) != 0) {
      i++;
    }
    add_history(line);
    free((void *)line);
    return l;
  }

  ~Editline() {}
};

#else
#include <histedit.h>
class Editline {
public:
  Editline() : m_hist_event(), m_editline(nullptr), m_history(nullptr) {

    m_editline = el_init("cmdb", stdin, stdout, stderr);
    el_set(m_editline, EL_PROMPT, &Editline::prompt);
    el_set(m_editline, EL_EDITOR, "emacs");

    m_history = history_init();
    if (m_history == nullptr) {
      std::cerr << "History could not be initialized\n";
    }

    history(m_history, &m_hist_event, H_SETSIZE, 800);
    history(m_history, &m_hist_event, H_LOAD, "~/.cmdb_history");
    el_set(m_editline, EL_HIST, history, m_history);
  }

  std::string GetLine() {
    char const *line;
    int count;
    line = el_gets(m_editline, &count);
    char c;
    int i = 0;
    while ((c = line[i]) != 0) {
      i++;
    }
    return line;
  }

  ~Editline() {
    history(m_history, &m_hist_event, H_SAVE, "~/.cmdb_history");
    history_end(m_history);
    el_end(m_editline);
  }

private:
  char const *prompt(EditLine *editline) {
    (void)editline;
    return "(cmdb) ";
  }

  HistEvent m_hist_event;
  EditLine *m_editline;
  History *m_history;
};
#endif
