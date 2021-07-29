/*
          _____   ____  _____            _____  
    /\   |  __ \ / __ \|  __ \     /\   |  __ \ 
   /  \  | |  | | |  | | |__) |   /  \  | |  | | Adorad - The Fast, Expressive & Elegant Programming Language
  / /\ \ | |  | | |  | |  _  /   / /\ \ | |  | | Languages: C, C++, and Assembly
 / ____ \| |__| | |__| | | \ \  / ____ \| |__| | https://github.com/adorad/adorad/
/_/    \_\_____/ \____/|_|  \_\/_/    \_\_____/ 

Licensed under the MIT License <http://opensource.org/licenses/MIT>
SPDX-License-Identifier: MIT
Copyright (c) 2021 Jason Dsouza <@jasmcaus>
*/

// Includes all the files needed for Adorad compilation

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <adorad/core/hcore.h> 

#include <adorad/compiler/types.h>
#include <adorad/compiler/tokens.h>  
#include <adorad/compiler/lexer.h>
#include <adorad/compiler/ast.h>
#include <adorad/compiler/parser.h>