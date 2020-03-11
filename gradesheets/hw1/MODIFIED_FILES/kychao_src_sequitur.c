--- hw1/dev_repo/solution/src/sequitur.c

+++ hw1/repos/kychao/hw1/src/sequitur.c

@@ -28,9 +28,9 @@

  * @param next  The symbol that is to be linked as the new next neighbor of this.

  */

 static void join_symbols(SYMBOL *this, SYMBOL *next) {

-    debug("Join %s%lu%s and %s%lu%s",

-	  IS_RULE_HEAD(this) ? "[" : "<", SYMBOL_INDEX(this), IS_RULE_HEAD(this) ? "]" : ">",

-	  IS_RULE_HEAD(next) ? "[" : "<", SYMBOL_INDEX(next), IS_RULE_HEAD(next) ? "]" : ">");

+    debug("Join %s%lu%s (%d) and %s%lu%s (%d)",

+	  IS_RULE_HEAD(this) ? "[" : "<", SYMBOL_INDEX(this), IS_RULE_HEAD(this) ? "]" : ">", this->value,

+	  IS_RULE_HEAD(next) ? "[" : "<", SYMBOL_INDEX(next), IS_RULE_HEAD(next) ? "]" : ">", next->value);

     if(this->next) {

 	// We will be assigning to this->next, which will destroy any digram

 	// that starts at this.  So that is what we have to delete from the table.

@@ -71,8 +71,8 @@

  * @param next  The symbol that is to be inserted.

  */

 void insert_after(SYMBOL *this, SYMBOL *next) {

-    debug("Insert symbol <%lu> after %s%lu%s", SYMBOL_INDEX(next),

-	  IS_RULE_HEAD(this) ? "[" : "<", SYMBOL_INDEX(this), IS_RULE_HEAD(this) ? "]" : ">");

+    debug("Insert symbol <%lu> (%d) after %s%lu%s (%d)", SYMBOL_INDEX(next), next->value,

+	  IS_RULE_HEAD(this) ? "[" : "<", SYMBOL_INDEX(this), IS_RULE_HEAD(this) ? "]" : ">", this->value);

     join_symbols(next, this->next);

     join_symbols(this, next);

 }

@@ -86,8 +86,8 @@

 static void delete_symbol(SYMBOL *this) {

     debug("Delete symbol <%lu> (value=%d)", SYMBOL_INDEX(this), this->value);

     if(IS_RULE_HEAD(this)) {

-	fprintf(stderr, "Attempting to delete a rule sentinel!\n");

-	abort();

+	   fprintf(stderr, "Attempting to delete a rule sentinel!\n");

+	   abort();

     }

     // Splice the symbol out, deleting the digram headed by the neighbor to the left.

     join_symbols(this->prev, this->next);

@@ -114,8 +114,8 @@

     debug("Expand last instance of underutilized rule [%lu] for %d",

 	   SYMBOL_INDEX(rule), rule->value);

     if(rule->refcnt != 1) {

-	fprintf(stderr, "Attempting to delete a rule with multiple references!\n");

-	abort();

+	   fprintf(stderr, "Attempting to delete a rule with multiple references!\n");

+	   abort();

     }

     SYMBOL *left = this->prev;

     SYMBOL *right = this->next;

@@ -158,7 +158,10 @@

 

     // Create a new nonterminal symbol that refers to the head of the rule

     // and insert it in place of the original digram.

+    debug("Value of rule to replace with: %d\n", rule->value);

     SYMBOL *new = new_symbol(rule->value, rule);

+    debug("Value of rule is now %d", new->value);

+

     insert_after(prev, new);

 

     // It might be the case that the insertion has created a second instance

@@ -174,6 +177,7 @@

     // have to check the digram starting at prev->next, which is still headed by the

     // nonterminal we just inserted.

     if(!check_digram(prev)) {

+        debug("wack");

 	check_digram(prev->next);

     }

 }

@@ -194,6 +198,7 @@

 	// If the digram headed by match constitutes the entire right-hand side

 	// of a rule, then we don't create any new rule.  Instead we use the

 	// existing rule to replace_digram for the newly inserted digram.

+    debug("wack");

 	rule = match->prev->rule;

 	replace_digram(this, match->prev->rule);

     } else {

@@ -203,9 +208,16 @@

 	// In fact, no digrams will be deleted during the construction of

 	// the new rule because the calls are being made in such a way that we are

 	// never overwriting any pointers that were previously non-NULL.

+    debug("Next nonterminal value: %d\n", next_nonterminal_value);

 	rule = new_rule(next_nonterminal_value++);

+

 	add_rule(rule);

+    debug("Added rule %d\n", next_nonterminal_value - 1);

+

+    debug("Adding symbol with rule %p", this->rule);

 	insert_after(rule->prev, new_symbol(this->value, this->rule));

+

+    debug("Adding symbol with rule %p", this->rule);

 	insert_after(rule->prev, new_symbol(this->next->value, this->next->rule));

 

 	// Now, replace the two existing instances of the right-hand side of the

@@ -214,10 +226,12 @@

 	// leading to their deletion from the hash table.

 	// They will potentially also cause the creation of digrams, due to the

 	// insertion of the nonterminal symbol.

-	// However, since the nonterminal symbol is a freshly created one that

 	// did not exist before, these replacements cannot result in the creation

 	// of digrams that duplicate already existing ones.

+    debug("SYMBOL INDEX MATCH IS %lu", SYMBOL_INDEX(match));

 	replace_digram(match, rule);

+

+    debug("SYMBOL INDEX THIS IS %lu\n", SYMBOL_INDEX(this));

 	replace_digram(this, rule);

 

 	// Insert the right-hand side of the new rule into the digram table.

@@ -245,8 +259,11 @@

     // performed recursively for that symbol and there is no need to do it here.

     // This is probably the most subtle point in the entire algorithm, which requires

     // substantial head-scratching to understand.

+    debug("rule->next->rule is %p", rule->next->rule);

+    debug("main_rule->prev is %lu", SYMBOL_INDEX(main_rule->prev));

 

     SYMBOL *tocheck = rule->next->rule;  // The first symbol of the just-added rule.

+

     if(tocheck) {

 	debug("Checking reference count for rule [%lu] => %d",

 	      SYMBOL_INDEX(tocheck), tocheck->refcnt);

@@ -275,16 +292,23 @@

  * @return  0 if no replacement was performed, nonzero otherwise.

  */

 int check_digram(SYMBOL *this) {

+    debug("Main_rule->prev is  is %lu", SYMBOL_INDEX(main_rule->prev));

     debug("Check digram <%lu> for a match", SYMBOL_INDEX(this));

 

     // If the "digram" is actually a single symbol at the beginning or

     // end of a rule, then there is no need to do anything.

-    if(IS_RULE_HEAD(this) || IS_RULE_HEAD(this->next))

-	return 0;

+    if(IS_RULE_HEAD(this) || IS_RULE_HEAD(this->next)) {

+        return 0;

+    }

 

     // Otherwise, look up the digram in the digram table, to see if there is

     // a matching instance.

+    debug("this->value is %d", this->value);

+    debug("this->next->value is %d", this->next->value);

+

     SYMBOL *match = digram_get(this->value, this->next->value);

+    debug("Match is %p", match);

+

     if(match == NULL) {

         // The digram did not previously exist -- insert it now.

 	digram_put(this);
