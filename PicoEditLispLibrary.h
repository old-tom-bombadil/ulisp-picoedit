/*
  PicoEdit LispLibrary - Version 1.1 - Feb 2026
  Hartmut Grawe - github.com/ersatzmoco - Jan 2026

  M5Cardputer editor version by hasn0life - Nov 2024 - https://github.com/hasn0life/ulisp-sedit-m5cardputer

  This uLisp version licensed under the MIT license: https://opensource.org/licenses/MIT
*/

const char LispLibrary[] PROGMEM = R"lisplibrary(

(backtrace t)

;
; Extended ULOS functions
;
;
; Define a class
(defun class (&optional parent slots constructor)
	#.(format nil "(class [parent] [slotlist] [constructor t/nil])~%ULOS function to define a new class or instance of a class.~%To define a new class, pass it to a variable, adding a list of property/method slots according to ULOS mechanism, optionally passing a parent class to inherit from.~%To define an instance, pass a parent class (= the prototype), omit the slots and set constructor to t.~%That way, the slots of the parent class are automatically copied into the instance.~%Setting the slots of the parent is equivalent to setting class variables, then.")
	(let ((obj (when parent (list (cons 'parent parent))))) 	
	 	(when (and constructor parent)
  		(when (symbolp parent) (setq parent (eval parent)))
  		(loop
	     (when (null parent) (return parent))
	     (unless (or (equal (search "_" (string (first parent))) 1) (search "parent" (string (first parent))) )
	      (push (cons (car (first parent)) (cdr (first parent))) obj))
	     (setq parent (cdr parent)))
  	)
    (loop
     (when (null slots) (return obj))
     (push (cons (first slots) (second slots)) obj)
     (setq slots (cddr slots)))
  )
)

; Get the value of a property slot in an instance/class or its parents
(defun gtv (obj slot)
	#.(format nil "(gtv obj 'slot)~%ULOS function to get value of property slot in obj.")
  (when (symbolp obj) (setq obj (eval obj)))
  (let ((pair (assoc slot obj)))
    (if pair (cdr pair)
           (let ((p (cdr (assoc 'parent obj))))
             (and p (gtv p slot))))
  )
)

; Update a property in an instance/class
(defun stv (obj slot value)
	#.(format nil "(stv obj 'slot value)~%ULOS function to set value of property slot in obj.")
  (when (symbolp obj) (setq obj (eval obj)))
  (let ((pair (assoc slot obj)))
    (when pair (setf (cdr pair) value))
  )
)

; Add property and method slots
(defun adp (obj slots)
	#.(format nil "(adp 'obj slotlist)~%ULOS function to add new property/method slots to obj, similar to JavaScript mechanism.")
	(let (newlist) 
    (loop
     (when (null slots) (return))
     (push (cons (first slots) (second slots)) newlist)
     (setq slots (cddr slots)))
    (set obj (append (eval obj) newlist)) 
  )
)

; Call a method in an object instance
;
(defun cmt (obj method &rest arguments)
	#.(format nil "(cmt obj 'method [arguments])~%ULOS function to call method of obj, providing a list of arguments.") 
	(apply (eval (gtv obj method)) (append (list obj) arguments))
)


;
; RGB helper function
;
;
(defun rgb (r g b)
	#.(format nil "(rgb r g b)~%Builds 16 bit color value from red, green and blue components provided as 8-bit integers.")
  (logior (ash (logand r #xf8) 8) (ash (logand g #xfc) 3) (ash b -3))
)


;
; LispBox screen editor
; Functions without built-in help to save memory.
;
;
(defun se:init (sk)
	(case sk 
		(t 
		 (defvar se:code_col (rgb 255 155 55))
		 (defvar se:line_col (rgb 90 60 30))
		 (defvar se:border_col (rgb 63 40 0))
		 (defvar se:bg_col (rgb 10 5 0))
		 (defvar se:cursor_col (rgb 180 80 0))
		 (defvar se:emph_col (rgb 0 128 0))
		 (defvar se:msg_col (rgb 0 190 0))
		 (defvar se:alert_col (rgb 255 0 0))
		 (defvar se:input_col (rgb 255 255 255))
		 (defvar se:help_col (rgb 255 127 0))
		 (defvar se:mark_col (rgb 90 40 0))
		)
	)

	(defvar se:origin (cons 20 10))
	(defvar se:txtpos (cons 0 0))
	(defvar se:lasttxtpos (cons 0 0))
	(defvar se:mark (cons nil nil))
	(defvar se:txtmax (cons 49 32))
	(defvar se:offset (cons 0 0))
	(defvar se:scrpos (cons 0 0))
	(defvar se:lastc nil)
	(defvar se:funcname nil)
	(defvar se:symname nil)
	(defvar se:filename nil)
	(defvar se:suffix nil)
	(defvar se:buffer ())
	(defvar se:bufbak ())
	(defvar se:editable t)
	(defvar se:curline "")
	(defvar se:copyline "")
	(defvar se:copybuf ())
	(defvar se:sniplist '("\"\"" "()" "(lambda ())" "(setf )" "(defvar )" "(let (()) )" "(when )" "(unless )" "(dotimes (i ) )" "#| |#"))
	(defvar se:tscale 1)
	(defvar se:leading (* 9 se:tscale))
	(defvar se:cwidth (* 6 se:tscale))
	(defvar se:openings ())
	(defvar se:closings ())
	(defvar se:lastmatch ())
	(defvar se:match nil)
	(defvar se:dirty nil)
	(defvar se:timestamp 0)
	(defvar se:idlestamp 0)
	(defvar se:exit nil)
	(defvar se:numtabs 2)
	(defvar se:help nil)
	(defvar se:lastsrch nil)
	(defvar se:markset nil)
	(defvar se:port 2)   #| software serial for PicoCalc (SerialPIO) - tied to GPIO 2 (TX) and 3 (RX) |#
	(defvar se:baud 96)

	(fill-screen)
	(draw-line 19 10 19 307 se:border_col)
	(draw-line 0 307 319 307 se:border_col)

	(se:info-line)
)

(defun se:info-line ()
	(fill-rect 0 0 320 10 0)
	(set-cursor 0 0)
	(set-text-color se:bg_col se:cursor_col)
	(write-text "F3 ld|F5 bnd|F6 dir|F9 del|F10 sav ")
)

(defun se:cleanup ()
	(makunbound 'se:buffer)
	(makunbound 'se:bufbak)
	(makunbound 'se:openings)
	(makunbound 'se:closings)
	(makunbound 'se:curline)
	(makunbound 'se:copyline)
	(makunbound 'se:sniplist)
	(makunbound 'se:copybuf)
	(gc)
)

(defun se:hide-cursor ()
	(when se:lastmatch
		(let ((spos nil) (bpos (car se:lastmatch)) (br (cdr se:lastmatch)))
			(setf spos (se:calc-scrpos bpos))
			(set-cursor (car spos) (cdr spos)) 
			(if (se:in-mark (cdr bpos))
				(set-text-color se:code_col se:mark_col)
				(set-text-color se:code_col se:bg_col)
			) 
			(write-text (string br))
			(setf se:lastmatch nil)
		)
	)
	(when se:lastc 
		(set-cursor (car se:scrpos) (cdr se:scrpos))
		(if (se:in-mark (cdr se:txtpos))
			(set-text-color se:code_col se:mark_col)
			(set-text-color se:code_col se:bg_col)
		) 
		(write-text (string se:lastc))
		(set-cursor 0 (cdr se:scrpos))
		(set-text-color se:line_col)
		(write-text (string (1+ (cdr se:txtpos))))
	)
)
		
(defun se:show-cursor (&optional forceb)
	(let ((x (car se:txtpos))
		  (y (cdr se:txtpos))
		  (ox (car se:offset))
		  (oy (cdr se:offset))
		  (padx (car se:origin))
		  (pady (cdr se:origin))
		  (myc (code-char 32)))
		(setf se:curline (nth y se:buffer))
		(setf se:scrpos (se:calc-scrpos se:txtpos))
		(set-cursor (car se:scrpos) (cdr se:scrpos))
		(set-text-color se:code_col se:cursor_col)
		#| check if cursor is within line string or behind last char |#
		(when (< x (length se:curline)) (setf myc (char se:curline x)))
		(setf se:lastc myc)
		(if forceb
			(progn
				(setf se:match t)
				(se:write-char (char-code myc))
				(setf se:match nil)
			)
			(se:write-char (char-code myc))
		)
		(set-cursor 0 (cdr se:scrpos))
		(set-text-color se:cursor_col)
		(write-text (string (1+ y)))
	)
)

(defun se:toggle-match ()
	(if se:match
		(progn
			(se:hide-cursor)
			(setf se:match nil) (setf se:openings ()) (setf se:closings ())
			(set-cursor 0 0)
			(set-text-color se:bg_col se:cursor_col)
			(write-text "F1")
			(se:show-cursor)
		)
		(progn
			(setf se:match t)
			(set-cursor 0 0)
			(set-text-color se:bg_col se:emph_col)
			(write-text "F1")
			(se:hide-cursor)
			(setf se:dirty t)
			(se:show-cursor)
		)
	)
)

(defun se:checkbr ()
	(se:hide-cursor)
	(se:map-brackets t)
	(se:show-cursor t)
	(setf se:openings ())
	(setf se:closings ())
	(setf se:match nil)
	(set-cursor 0 0)
	(set-text-color se:bg_col se:cursor_col)
	(write-text "F1")
)

(defun se:map-brackets (&optional forcemp)	
	(when (or se:match forcemp)
		(gc t)
		(setq se:openings nil)
		(setq se:closings nil)
		(let ((myline "") (keys ()) (octr 0) (y 0))
			(dolist (myline se:buffer)
				(dotimes (x (length myline))
					(when (eq (char myline x) #\040) (push (cons (cons x y) octr) se:openings) (push octr keys) (incf octr))
					(when (eq (char myline x) #\041) (push (cons (cons x y) (if keys (pop keys) nil)) se:closings))
				)
				(incf y)
			)
		)
	)
)

(defun se:find-closing-bracket ()
	(let ((opentry (assoc* se:txtpos se:openings #'equal)) (clentry nil))
		(if opentry
			(progn
				(setf clentry (reverse-assoc* (cdr opentry) se:closings #'equal))
				(if clentry
					clentry
					nil
				)
			)
			nil
		)
	)
)

(defun se:find-opening-bracket ()
	(let ((clentry (assoc* se:txtpos se:closings #'equal)) (opentry nil))
		(if clentry
			(progn
				(setf opentry (reverse-assoc* (cdr clentry) se:openings #'equal))
				(if opentry
					opentry
					nil
				)
			)
			nil
		)
	)
)

(defun se:in-window (pos)
	(if (and (>= (car pos) (car se:offset)) (<= (car pos) (+ (car se:offset) (car se:txtmax))))
		(if (and (>= (cdr pos) (cdr se:offset)) (<= (cdr pos) (+ (cdr se:offset) (cdr se:txtmax))))
			t
			nil
		)
		nil
	)
)

(defun se:move-window (&optional forceshow)
	(let ((x (car se:txtpos))
		  (y (cdr se:txtpos))
		  (ox (car se:offset))
		  (oy (cdr se:offset))
		  (xm (car se:txtmax))
		  (ym (cdr se:txtmax)))
		(when (> x (+ ox xm))
			(setf (car se:offset) (+ (car se:offset) (- x (+ ox xm))))
			(setf forceshow t)
		)
		(when (> y (+ oy ym))
		    (setf (cdr se:offset) (+ (cdr se:offset) (- y (+ oy ym))))
			(setf forceshow t)
		)
		(when (< x ox)
			(setf (car se:offset) (- (car se:offset) (- ox x)))
			(setf forceshow t)
		)
		(when (< y oy)
			(setf (cdr se:offset) (- (cdr se:offset) (- oy y)))
			(setf forceshow t)
		)
		(when forceshow
			(se:show-text)
		)
	)
)

(defun se:calc-scrpos (tpos)
	(let ((sx 0) (sy 0) (ox (car se:offset)) (oy (cdr se:offset)) (padx (car se:origin)) (pady (cdr se:origin)))
		(setf sx (+ (* (- (car tpos) ox) se:cwidth) padx))
		(setf sy (+ (* (- (cdr tpos) oy) se:leading) pady))
		(cons sx sy)
	)
)

(defun se:calc-msgpos (tpos)
	(let ((sx 0) (sy 0) (padx (car se:origin)) (pady (cdr se:origin)))
		(setf sx (+ (* (car tpos) se:cwidth) padx))
		(setf sy (+ (* (cdr tpos) se:leading) pady))
		(cons sx sy)
	)
)

(defun se:write-char (cc)
	(let ((bpos nil) (spos nil))
		(cond
			((and (= cc 40) se:match)
				(when se:dirty (se:map-brackets) (setf se:dirty nil))
				(setf bpos (se:find-closing-bracket))
				(when bpos
					(when (se:in-window bpos)
						(setf spos (se:calc-scrpos bpos))
						(set-cursor (car spos) (cdr spos))
						(set-text-color se:code_col se:emph_col)
						(write-char 41 )
						(setf se:lastmatch (cons bpos ")"))
					)
					(set-text-color se:code_col se:emph_col)
				)
				(set-cursor (car se:scrpos) (cdr se:scrpos))
				(write-char cc)
				(set-text-color se:code_col se:bg_col)
			)
			((and (= cc 41) se:match)
				(when se:dirty (se:map-brackets) (setf se:dirty nil))
				(setf bpos (se:find-opening-bracket))
				(when bpos
					(when (se:in-window bpos)
						(setf spos (se:calc-scrpos bpos))
						(set-cursor (car spos) (cdr spos))
						(set-text-color se:code_col se:emph_col)
						(write-char 40 )
						(setf se:lastmatch (cons bpos "("))
					)
					(set-text-color se:code_col se:emph_col)
				)
				(set-cursor (car se:scrpos) (cdr se:scrpos))
				(write-char cc)
				(set-text-color se:code_col se:bg_col)
			)
			(t (write-char cc))
		)
	)
)

(defun se:disp-line (y)
	(let ((ypos (+ (cdr se:origin) (* (- y (cdr se:offset)) se:leading))) (myl " ") (myy (nth y se:buffer)))
		(when myy (setf myl (concatenate 'string myy myl)))
			(set-text-color se:line_col)
			(set-cursor 0 ypos)
			(write-text (string (1+ y)))

		(set-cursor (car se:origin) ypos)
		(if (se:in-mark y)
			(set-text-color se:code_col se:mark_col)
			(set-text-color se:code_col se:bg_col)
		)
		(when (> (length myl) (car se:offset))
			(write-text (subseq myl (car se:offset) (min (length myl) (+ (car se:txtmax) (car se:offset) 1))))
		)
	)
)

(defun se:show-text ()
	(fill-rect 20 10 300 297 se:bg_col)
	(fill-rect 0 10 19 297 se:bg_col)
	(let ((i 0) (ymax (min (cdr se:txtmax) (- (length se:buffer) (cdr se:offset) 1))))
		(loop
			(se:disp-line (+ i (cdr se:offset)))
			(when (= i ymax) (return))
			(incf i)
		)
	)
)

(defun se:show-dir ()
	(se:hide-cursor)
	(if se:editable
		(progn
			(setf se:editable nil)
			(se:save-buffer)
			(let ((dirbuf (sd-card-dir 2)))
				(se:compile-dir dirbuf 0)
				(setq se:buffer (reverse se:buffer))
			)
			(setf se:dirty t)
			(se:move-window t)
			(set-cursor (* 35 se:cwidth) 0)
			(set-text-color se:alert_col se:line_col)
			(write-text "SD DIRECTORY")
		)
		(se:restore-buffer)
	)
	(se:show-cursor)
)

(defun se:compile-dir (mydir tab)
	(let ((tabstr ""))
		(dotimes (i tab)
			(setf tabstr (concatenate 'string tabstr " "))
		)
		(dolist (entry mydir)
			(if (listp entry)
				(se:compile-dir entry (+ tab 3))
				(push (concatenate 'string tabstr entry) se:buffer)
			)
		)
	)
)

(defun se:show-help ()
	(se:hide-cursor)
	(if se:editable
		(progn
			(setf se:editable nil)
			(setf se:help t)
			(se:save-buffer)
			(setf se:buffer nil)

			(let ((result nil))
				(setf result (se:input "Press key [ENTER]: " nil 1))
				(se:clr-msg)
				(if result
					(progn
						(setf se:buffer (sort (mapcar princ-to-string (se:ref result)) string<))
						(when se:buffer (se:msg "Select entry with cursor, then [ENTER]."))
						(unless se:buffer (setf se:buffer '("No entry")))
						(se:move-window t)
						(set-cursor (* 35 se:cwidth) 0)
						(set-text-color se:alert_col se:line_col)
						(write-text "HELP")
					)
					(se:restore-buffer)
				)
			)
		)
		(se:restore-buffer)
	)
	(se:show-cursor)
)

(defun se:show-doc ()
	(se:clr-msg)
	(se:hide-cursor)
	(setf se:curline (nth (cdr se:txtpos) se:buffer))
	(setf se:buffer nil)
	(if (eval (read-from-string se:curline))
		(let ((docstring (documentation (read-from-string se:curline)))
					(parbuf nil) (wordbuf nil) (lastword nil) (docline ""))
			(if (< (length docstring) 1)
				(setf se:buffer '("No doc"))
				(progn
					(setf parbuf (split-string-to-list (string #\Newline) docstring))
					(push (pop parbuf) se:buffer)
					(dolist (p parbuf)
						(setf lastword nil)
						(setf docline "")
						(setf wordbuf (split-string-to-list " " p))
						(dolist (w wordbuf)
							(if (< (+ (length w) (length docline)) 49)
								(progn
									(setf docline (concatenate 'string docline w " "))
									(setf lastword nil)
								)
								(progn
									(push docline se:buffer)
									(setf docline (concatenate 'string w " "))
									(setf lastword w)
								)
							)
						)
						(if lastword
							(push lastword se:buffer)
							(push docline se:buffer)
						)
					)
					(setf se:buffer (reverse se:buffer))
				)
			)
		)
		(setf se:buffer '("No doc"))
	)
	(setf se:txtpos (cons 0 0))
	(se:move-window t)
	(setf se:help nil)
	(se:show-cursor)
)

(defun se:ref (chr)
		(mapcan 
			(lambda (x)
				(let ((entry ""))
					(when x
						(setf entry (search chr (string x)))
						(when entry
							(when (and (< entry 1) (> (length (string x)) 0)) (list x))
						)
					)
				)
			) 
			(apropos-list chr)
		)
)


(defun se:flush-buffer ()
	(when se:editable
		(when (se:alert "Flush buffer")
			(se:hide-cursor)
			(setf se:dirty nil)
			(setq se:buffer (list ""))
			(setf se:txtpos (cons 0 0))
			(setf se:offset (cons 0 0))
			(setf se:filename nil)
			(setf se:funcname nil)
			(se:info-line)
			(se:show-text)
			(se:show-cursor)
		)
	)
)

(defun se:flush-line ()
	(when se:editable
		(se:hide-cursor)
		(setf se:dirty t)
		(if se:markset
			(let* ((start (car se:mark)) (end (cdr se:mark)) (numl (- (1+ end) start)) )
				(setf se:copybuf ())
				(dotimes (ln numl)
					(push (nth (- end ln) se:buffer) se:copybuf)
					(setf (nth (- end ln) se:buffer) "")
				)
				(se:unmark)
			)
			(let* ((x (car se:txtpos))
			   (y (cdr se:txtpos))
			   (myl se:curline)
			   (firsthalf ""))
			  (setf se:copybuf ())
				(setf firsthalf (subseq myl 0 x))
				(setf se:copyline (subseq myl x (length myl)))
				(setf (nth y se:buffer) firsthalf)
				(setf se:curline (concatenate 'string (nth y se:buffer) " "))
				(setf se:lastc nil)
				(se:disp-line y)
			)
		)
		(setf se:dirty t)
		(se:show-text)
		(se:show-cursor)
	)
)

(defun se:insert (newc)
	(when se:editable
		(se:hide-cursor)
		(setf se:dirty t)
		(let* ((x (car se:txtpos))
			   (y (cdr se:txtpos))
			   (myl se:curline)
			   (firsthalf "")
			   (scdhalf ""))
			(setf firsthalf (subseq myl 0 x))
			(setf scdhalf (subseq myl x))
			(setf myl (concatenate 'string firsthalf (string newc) scdhalf))
			(setf (nth y se:buffer) myl)
			(incf (car se:txtpos))
			(setf se:curline myl)
			(setf se:lastc nil)
			(if (> (car se:txtpos) (car se:txtmax)) (se:move-window t) (se:disp-line y))
		)
		#| (se:show-cursor) |#
	)
)

(defun se:tab (numtabs)
		(dotimes (i numtabs)
			(se:insert #\032)
		)
)

(defun se:enter ()
	(when se:editable
		(se:hide-cursor)
		(setf se:dirty t)
		(let* ((x (car se:txtpos))
			   (y (cdr se:txtpos))
			   (myl se:curline)
			   (firsthalf "")
			   (scdhalf "")
			   (newl () ))
			(setf firsthalf (subseq myl 0 x))
			(setf scdhalf (subseq myl x))
			(dotimes (i y)
				(push (nth i se:buffer) newl)
			)
			(push firsthalf newl)
			(push scdhalf newl)
			(dotimes (i (- (length se:buffer) (1+ y)))
				(push (nth (+ i (1+ y)) se:buffer) newl)
			)
			(setf se:buffer (reverse newl))
			(setf (car se:txtpos) 0)
			(incf (cdr se:txtpos))
			(setf se:curline (nth (1+ y) se:buffer))
			(setf se:lastc nil)
			(setf (car se:offset) 0)
			(se:move-window t)
		)
		#| (se:show-cursor) |#
	)
)

(defun se:delete ()
	(when se:editable
		(se:hide-cursor)
		(setf se:dirty t)
		(let* ((x (car se:txtpos))
			   (y (cdr se:txtpos))
			   (myl se:curline)
			   (firsthalf "")
			   (scdhalf ""))
			(if (> x 0)
				(progn
					(setf firsthalf (subseq myl 0 (1- x)))
					(setf scdhalf (subseq myl x))
					(setf myl (concatenate 'string firsthalf scdhalf))
					(setf (nth y se:buffer) myl)
					(setf se:curline (concatenate 'string myl " "))
					(decf (car se:txtpos))
					(setf se:lastc nil)
					(if (> (car se:offset) 0)
						(se:move-window t)
						(se:disp-line y)
					)
				)
				(when (> y 0)
					(setf scdhalf se:curline)
					(setf se:buffer (remove y se:buffer))
					(decf (cdr se:txtpos))
					(decf y)
					(setf se:curline (nth y se:buffer))
					(setf firsthalf se:curline)
					(if firsthalf
						(progn
							(setf (nth y se:buffer) (concatenate 'string firsthalf scdhalf))
							(setf (car se:txtpos) (length firsthalf))
						)
						(progn
							(setf (nth y se:buffer) scdhalf)
							(setf (car se:txtpos) 0)
						)
					)
					(se:move-window t)
				)
			)
		)
		#| (se:show-cursor) |#
	)
)

(defun se:copy ()
	(if se:markset
		(let* ((start (car se:mark)) (end (cdr se:mark)) (numl (- (1+ end) start)) )
			(setf se:copybuf ())
			(dotimes (ln numl)
				(push (nth (- end ln) se:buffer) se:copybuf)
			)
			(se:unmark)
		)
		(progn
			(setf se:copybuf ())
			(setf se:copyline se:curline)
		)
	)
)

(defun se:paste ()
	(when se:editable
		(setf se:dirty t)
		(if se:copybuf
			(let ((firstline t) (y (cdr se:txtpos)))
				(dolist (ln se:copybuf)
					(if firstline
						(progn
							(dotimes (i (length ln))
								(se:insert (char ln i))
							)
							(setf firstline nil)
						)
						(progn
							(se:enter)
							(incf y)
							(setf (nth y se:buffer) ln)
							(setf (car se:txtpos) (length ln))
							(setf se:curline ln)
							(setf se:lastc nil)
						)
					)
				)
				(se:move-window t)
				(se:show-cursor)
			)
			(when se:copyline
				(dotimes (i (length se:copyline))
					(se:insert (char se:copyline i))
				)
			)	
		)
	)
)

(defun se:snippet (num)
	(let ((sn (nth num se:sniplist)))
		(when (> (length sn) 0)
			(dotimes (i (length sn))
				(se:insert (char sn i))
			)
		)
	)
)

(defun se:left ()
	(se:hide-cursor)
	(cond
		#| xpos > 0 |#
		((> (car se:txtpos) 0) 
			(decf (car se:txtpos))
			(se:move-window)
		)
		#| xpos == 0, but ypos > 0 |#
		((> (cdr se:txtpos) 0)
			(decf (cdr se:txtpos))
			(setf (car se:txtpos) (length (nth (cdr se:txtpos) se:buffer)))
			(se:move-window)
		)
	)
	(se:show-cursor)
)

(defun se:right ()
	(se:hide-cursor)
	(cond
		#| xpos < eol |#
		((< (car se:txtpos) (length se:curline)) 
			(incf (car se:txtpos))
			(se:move-window)
		)
		#| xpos == eol, but ypos < end of se:buffer |#
		((< (cdr se:txtpos) (1- (length se:buffer)))
			(incf (cdr se:txtpos))
			(setf (car se:txtpos) 0)
			(se:move-window)
		)
	)
	(se:show-cursor) 
)

(defun se:up ()
	(se:hide-cursor)
	(cond
		#| ypos > 0 |#
		((> (cdr se:txtpos) 0) 
			(decf (cdr se:txtpos))
			(setf se:curline (nth (cdr se:txtpos) se:buffer))
			(when (> (car se:txtpos) (length se:curline)) 
				(setf (car se:txtpos) (length se:curline))
			)
			(se:move-window)
		)
	)
	(se:show-cursor)
)

(defun se:down ()
	(se:hide-cursor)
	(cond
		#| ypos < length of se:buffer |#
		((< (cdr se:txtpos) (1- (length se:buffer))) 
			(incf (cdr se:txtpos))
			(setf se:curline (nth (cdr se:txtpos) se:buffer))
			(when (> (car se:txtpos) (1+ (length se:curline))) (setf (car se:txtpos) (length se:curline)))
			(se:move-window)
		)
	)
	(se:show-cursor)
)

(defun se:linestart ()
	(se:hide-cursor)
	(setf (car se:txtpos) 0)
	(se:move-window)
	(se:show-cursor)
)

(defun se:lineend ()
	(se:hide-cursor)
	(setf (car se:txtpos) (length se:curline))
	(se:move-window)
	(se:show-cursor)
)

(defun se:nextpage ()
	(se:hide-cursor)
	(setf (cdr se:txtpos) (min (length se:buffer) (+ (cdr se:txtpos) (cdr se:txtmax) 1)))
	(se:move-window)
	(se:show-cursor)
)

(defun se:prevpage ()
	(se:hide-cursor)
	(setf (cdr se:txtpos) (max 0 (- (cdr se:txtpos) (cdr se:txtmax) 1)))
	(se:move-window)
	(se:show-cursor)
)

(defun se:docstart ()
	(se:hide-cursor)
	(setf se:txtpos (cons 0 0))
	(se:move-window)
	(se:show-cursor)
)

(defun se:search ()
	(se:hide-cursor)
	(let* ((srchstr (se:input "SEARCH for: " se:lastsrch 40)) 
		(found nil) (fx nil) (fy (cdr se:txtpos)) 
		(srchcell se:buffer))
		(setf se:lastsrch srchstr)
		(when srchstr
			(dotimes (i fy) (setf srchcell (cdr srchcell)))
			(loop
				(setf fx (search srchstr (car srchcell)))
				(when fx
					(return)
				)
				(incf fy)
				(setf srchcell (cdr srchcell))
				(unless srchcell (return))
			)
		)
		(if fx
			(progn
				(setf se:txtpos (cons fx fy))
				(se:move-window t)
			)
			(progn
				(se:msg "No match.")
				(delay 2000)
			)
		)
	)
	(se:clr-msg)
	(se:show-cursor)
)

(defun se:mark-in ()
	(se:hide-cursor)
	(setf (car se:mark) (cdr se:txtpos))
	(when (cdr se:mark)
		(when (< (cdr se:mark) (car se:mark)) (setf (car se:mark) (cdr se:mark)) (setf (cdr se:mark) (cdr se:txtpos)))
	)
	(when (se:checkmark) (se:move-window t)	)
	(se:show-cursor)
)

(defun se:mark-out ()
	(se:hide-cursor)
	(setf (cdr se:mark) (cdr se:txtpos))
	(when (car se:mark)
		(when (< (cdr se:mark) (car se:mark)) (setf (cdr se:mark) (car se:mark)) (setf (car se:mark) (cdr se:txtpos)))
	)
	(when (se:checkmark) (se:move-window t)	)
	(se:show-cursor)
)

(defun se:unmark ()
	(when se:markset
		(se:hide-cursor)
		(setf se:mark (cons nil nil))
		(se:checkmark)
		(se:move-window t)
		(se:show-cursor)
	)
)

(defun se:checkmark ()
	(if (and (car se:mark) (cdr se:mark))
		(setf se:markset t)
		(setf se:markset nil)
	)
	se:markset
)

(defun se:in-mark (y)
	(if se:markset
		(if (and (>= y (car se:mark)) (<= y (cdr se:mark)))
			t
			nil
		)
		nil
	)
)

(defun se:run ()
	(when se:editable
		(let ((body "") (fname (se:input "Symbol name: " se:funcname 40)))
			(mapc (lambda (x) (setf body (concatenate 'string body x))) se:buffer)
			(if (> (length fname) 0)
				(when (se:alert (concatenate 'string "Bind code to symbol " fname " "))
					(eval (read-from-string (concatenate 'string (format nil "(defvar ~a" fname) (format nil " '~a)" body))))
					(se:msg "Done! Returning to REPL")
					(delay 2000)
					(se:clr-msg)
					(se:cleanup)
					(setf se:exit t)
				)
				(eval (read-from-string body))
			)
		)
	)
)

(defun se:execute ()
	(if se:markset
		(let* ((eline "(progn ") (start (car se:mark)) (end (cdr se:mark)) 
				(numl (- (1+ end) start)))
			(dotimes (ln numl)
				(setf eline (concatenate 'string eline (nth (+ start ln) se:buffer)))
			)
			(setf eline (concatenate 'string eline ")" ))
			(eval (read-from-string eline))
		)
		(eval (read-from-string (nth (cdr se:txtpos) se:buffer)))
	)
)

(defun se:sendserial ()
	(se:hide-cursor)
	(keyboard-key-pressed)
	(delay 100)
	(keyboard-key-pressed)
	(if se:editable
		(let* ((eline "(progn " ) (start (car se:mark)) (end (cdr se:mark)) 
					(numl 1) (rbyte nil) (rline "") (firstbyte t) (stripped nil))
			(setf se:editable nil)
			(with-serial (str se:port se:baud)
				(if se:markset
					(progn
						(setf numl (- (1+ end) start))
						(dotimes (ln numl)
							(setf eline (concatenate 'string eline (nth (+ start ln) se:buffer)))
						)
						(setf eline (concatenate 'string eline ")" ))
						(princ eline str) 
						(terpri str)
					)
					(progn
						(princ (nth (cdr se:txtpos) se:buffer) str)
						(terpri str)
					)
				)
				(se:msg "SENT! Waiting for answer ...")
				(se:save-buffer)
				(set-cursor (* 35 se:cwidth) 0)
				(set-text-color se:alert_col se:line_col)
				(write-text "SERIAL ANSW")
				(loop 
					(setf rbyte (read-byte str))
					(when rbyte
						(if (= rbyte 10)
							(progn
								(when firstbyte
									(se:clr-msg)
									(se:msg "Incoming ... press key to stop listening")
									(setf firstbyte nil)
								)
								(setf se:buffer (append se:buffer (list rline)))
								(se:move-window t)
								(setf rline "")
							)
							(unless (< rbyte 32)
								(setf rline (concatenate 'string rline (string (code-char rbyte))))
							)
						)
						(setf rbyte nil)
					)
					(when (keyboard-key-pressed) (return))
				)
			(se:clr-msg)
			(keyboard-key-pressed)
			)
		)
		(se:restore-buffer)
	 )
	(se:show-cursor)
)

(defun se:remove ()
	(let ((fname (se:input "DELETE file name: " nil 17 t)) (suffix (se:input "Suffix: ." "CL" 3 t)))
		(if (sd-file-exists (concatenate 'string fname "." suffix))
			(when (se:alert (concatenate 'string "Delete file " fname "." suffix))
				(sd-file-remove (concatenate 'string fname "." suffix))
				(se:msg "Done! Returning to editor.")
				(delay 1000)
				(se:clr-msg)
			)
		)
	)
)

(defun se:save ()
	(unless se:suffix (setf se:suffix "CL"))
	(let ((fname (se:input "SAVE file name: " se:filename 17 t)) (suffix (se:input "Suffix: ." se:suffix 3 t)) (overwrite t) (fullname ""))
		(setf fullname (concatenate 'string fname "." suffix))		
		(if (sd-file-exists fullname)
			(unless (se:alert "File exists. Overwrite")
				(setf overwrite nil)
			)
		)
		(unless (or (< (length fname) 1) (< (length suffix) 1) (not overwrite))
			(when overwrite (sd-file-remove fullname))
			(with-sd-card (strm fullname 2)
				(dolist (line se:buffer)
					(princ line strm)
					(princ (code-char 10) strm)
				)
			)
			(se:info-line)
			(set-cursor (* 35 se:cwidth) 0)
			(set-text-color se:alert_col se:line_col)
			(setf fname (concatenate 'string fname "." suffix))
			(if (> (length fname) 13)
				(write-text (concatenate 'string "FILE " (subseq fname 0 10) "...") )
				(write-text (concatenate 'string "FILE " fname))
			)
			(se:msg "Done! Returning to editor.")
			(delay 1000)
			(se:clr-msg)
		)
	)
)

(defun se:quicksave ()
	(when se:editable
		(sd-file-remove "lisp/BACKUP.CL")
		(with-sd-card (strm "lisp/BACKUP.CL" 2)
			(dolist (line se:buffer)
				(princ line strm)
				(princ (code-char 10) strm)
			)
		)
		(se:msg "SAVED to BACKUP.CL")
		(delay 500)
		(se:clr-msg)
	)
)

(defun se:load ()
	(when se:editable
		(when (se:alert "Discard buffer and load from SD")
			(let ((fname (se:input "LOAD file name: " nil 17 t)) (suffix (se:input "Suffix: ." "CL" 3 t)) (line ""))
				(unless (or (< (length fname) 1) (< (length suffix) 1) (not (sd-file-exists (concatenate 'string fname "." suffix))))
					(se:unmark)
					(setq se:buffer ())
					(setf se:funcname nil)
					(with-sd-card (strm (concatenate 'string fname "." suffix) 0)
						(loop
							(setf line (read-line strm))
							(if line 
								(push line se:buffer)
								(return)
							)
						)
					)
					(setf se:buffer (reverse se:buffer))
					(se:hide-cursor)
					(setf se:dirty t)
					(se:info-line)
					(set-cursor (* 35 se:cwidth) 0)
					(set-text-color se:alert_col se:line_col)
					(setf fname (concatenate 'string fname "." suffix))
					(if (> (length fname) 13)
						(write-text (concatenate 'string "FILE " (subseq fname 0 10) "...") )
						(write-text (concatenate 'string "FILE " fname))
					)
					(setf se:txtpos (cons 0 0))
					(setf se:offset (cons 0 0))
					(se:show-text)
					(se:show-cursor)
				)
			)
		)
	)
)

(defun se:save-buffer ()
	(se:unmark)
	(setq se:bufbak (copy-list se:buffer))
	(setf se:lasttxtpos (cons (car se:txtpos) (cdr se:txtpos)))
	(setf se:txtpos (cons 0 0))
	(setq se:buffer ())
	(se:info-line)
	(when se:match
		(set-cursor 0 0)
		(set-text-color se:bg_col se:emph_col)
		(write-text "F1")
	)
)

(defun se:restore-buffer ()
	(se:unmark)
	(setf se:help nil)
	(se:clr-msg)
	(setq se:buffer (copy-list se:bufbak))
	(setq se:bufbak nil)
	(setf se:dirty t)
	(se:info-line)
	(if (or se:funcname se:filename)
		(let ((myname "") (mytype ""))
			(if se:funcname
				(progn (setf myname se:funcname) (setf mytype "SYM: "))
				(progn (setf myname se:filename) (setf mytype "FILE: "))
			)
			(set-cursor (* 35 se:cwidth) 0)
			(set-text-color se:code_col se:cursor_col)
			(if (> (length myname) 13)
				(write-text (concatenate 'string mytype (subseq myname 0 10) "..."))
				(write-text (concatenate 'string mytype myname))
			)
		)
		(se:info-line)
	)
	(when se:match
		(set-cursor 0 0)
		(set-text-color se:bg_col se:emph_col)
		(write-text "F1")
	)
	(setf se:txtpos (cons (car se:lasttxtpos) (cdr se:lasttxtpos)))
	(se:move-window t)
	(setf se:editable t)
)

(defun se:viewsym ()
	(se:hide-cursor)
	(if se:editable
		(let ((symname (se:input "VIEW SYM: " nil 40)))
			(when (and (> (length symname) 0) (boundp (read-from-string symname)))
				(setf se:editable nil)
				(se:save-buffer)
				(setq se:buffer (cdr (split-string-to-list (string #\Newline) (with-output-to-string (str) (pprint (eval (read-from-string symname)) str)))))
				(setf se:dirty t)
				(se:move-window t)
				(set-cursor (* 35 se:cwidth) 0)
				(set-text-color se:alert_col se:line_col)
				(if (> (length symname) 14)
					(write-text (concatenate 'string "SYM " (subseq symname 0 11) "..."))
					(write-text (concatenate 'string "SYM " symname))
				)
			)
		)
		(se:restore-buffer)
	)
	(se:show-cursor)
)

(defun se:loadview ()
	(se:hide-cursor)
	(if se:editable
		(let ((fname (se:input "VIEW file: " nil 17 t)) (suffix (se:input "Suffix: ." "CL" 3 t)) (line ""))
			(unless (or (< (length fname) 1) (< (length suffix) 1) (not (sd-file-exists (concatenate 'string fname "." suffix))))
				(setf se:editable nil)
				(se:save-buffer)
				(with-sd-card (strm (concatenate 'string fname "." suffix) 0)
					(loop
						(setf line (read-line strm))
						(if line 
							(push line se:buffer)
							(return)
						)
					)
				)
				(setf se:buffer (reverse se:buffer))
				(setf se:dirty t)
				(se:move-window t)
				(se:info-line)
				(set-cursor (* 35 se:cwidth) 0)
				(set-text-color se:alert_col se:line_col)
				(setf fname (concatenate 'string fname "." suffix))
				(if (> (length fname) 13)
					(write-text (concatenate 'string "FILE " (subseq fname 0 10) "...") )
					(write-text (concatenate 'string "FILE " fname))
				)
			)
		)
		(se:restore-buffer)
	)
	(se:show-cursor)
)

(defun se:alert (mymsg)
	(se:msg (concatenate 'string mymsg " y/n ?") t)
	(let ((lk nil))
		(loop
			(when lk (return))
			(setf lk (keyboard-get-key))
		)
		(se:clr-msg)
		(if (or (= lk 121) (= lk 89))
			t
			nil
		)
	)
)

(defun se:msg (mymsg &optional alert cursor)
	(se:clr-msg)
	(if alert
		(set-text-color se:alert_col)
		(set-text-color se:msg_col)
	)
	(let ((spos (se:calc-msgpos (cons 0 (1+ (cdr se:txtmax))))))
		(set-cursor (+ (car spos) 2) (+ (cdr spos) 2))
		(write-text mymsg)
	)
	(when cursor
		(setf cursor (max 0 cursor))
		(let ((spos (se:calc-msgpos (cons cursor (1+ (cdr se:txtmax))))))
			(set-text-color se:code_col se:emph_col)
			(set-cursor (+ (car spos) 2) (+ (cdr spos) 2))
			(write-text (subseq mymsg cursor (1+ cursor)))
		)
	)
)

(defun se:input (mymsg default maxlen &optional chkanum)
	(let* ((ibuf "") (pressedkey nil)
		(lshift nil) (rshift nil) (ctrl nil) (alt nil) (caps nil)
		(lpos (1+ (cdr se:txtmax)))
		(istart (length mymsg))
		(iend (min maxlen (- (car se:txtmax) istart 1)))
		(ipos 0))
		(when default (setf ibuf default))
		(se:msg (concatenate 'string mymsg ibuf " ") nil istart)
		(loop
			(setf pressedkey (keyboard-get-key))
			(when pressedkey
				(case pressedkey
					(162 (setf lshift t))
					(163 (setf rshift t))
					(165 (setf ctrl t))
					(161 (setf alt t))
					(193 (setf caps t))
					(180 (when (> ipos 0) (decf ipos)))
					(183 (when (< ipos (length ibuf)) (incf ipos)))
					(10 (se:clr-msg)  (return ibuf))
					((8 212) (if (> ipos 0)
							(progn
								(decf ipos)
								(setf ibuf (concatenate 'string (subseq ibuf 0 ipos) (subseq ibuf (1+ ipos))))
							)
							(setf ibuf "")
						 ) 
					)
					(t
						(if chkanum 
							(when (and (< (length ibuf) maxlen) (se:alphnum pressedkey))
								(setf ibuf (concatenate 'string (subseq ibuf 0 ipos) (string (code-char pressedkey)) (subseq ibuf ipos (length ibuf))))
								(incf ipos)
							)
							(when (< (length ibuf) maxlen)
								(setf ibuf (concatenate 'string (subseq ibuf 0 ipos) (string (code-char pressedkey)) (subseq ibuf ipos (length ibuf))))
								(incf ipos)
							)
						)
					)
				)
				(setf lshift nil rshift nil ctrl nil alt nil caps nil)
				(se:clr-msg)
				(se:msg (concatenate 'string mymsg ibuf " ") nil (+ istart ipos))
			)
		)
	)
)

(defun se:alphnum (chr)
	(if (or (= chr 32) (= chr 45) (= chr 47) (= chr 95) (and (> chr 64) (< chr 91)) (and (> chr 96) (< chr 123)) (and (> chr 47) (< chr 58)))
		t
		nil 
	)
)

(defun se:clr-msg ()
	(fill-rect 20 308 300 9 se:bg_col)
)

(defun se:sedit (&optional myform myskin)
	#.(format nil "(se:sedit ['symbol])~%Invoke fullscreen editor, optionally providing the name of a bound symbol to be edited.")
	(se:init myskin)
	(let* ((pressedkey nil)
			(lshift nil) (rshift nil) (ctrl nil) (alt nil) (now 0) (reqkey nil))
			(if myform
				(progn
					(setf se:funcname (prin1-to-string myform)) 
					(setq se:buffer (cdr (split-string-to-list (string #\Newline) (with-output-to-string (str) (pprint (eval myform) str)))))
					(set-cursor (* 35 se:cwidth) 0)
					(set-text-color se:code_col se:cursor_col)
					(if (> (length se:funcname) 13)
						(write-text (concatenate 'string "SYM " (subseq se:funcname 0 10) "..."))
						(write-text (concatenate 'string "SYM " se:funcname))
					)
				)
				(setq se:buffer (list ""))
			)
			(setf se:dirty t)
			(se:show-text)
			(se:show-cursor)
			(setf se:timestamp (millis))
			(setf se:idlestamp (millis))
			(loop
				(setf now (millis))
				(when (> (abs (- now se:timestamp)) 10)
					(setf se:timestamp now)
					(setf reqkey (keyboard-key-pressed))
					(when reqkey
						(unless (eq (second reqkey) 3)
							(setf pressedkey (first reqkey))
							(setf se:idlestamp (millis))
							(if (or lshift rshift ctrl alt)
								(progn 
										(when (or lshift rshift)
											(if (and (> pressedkey 31) (< pressedkey 127))
												(se:insert (code-char pressedkey))
												(case pressedkey
													(210 (se:linestart))
													(213 (se:lineend))
													(181 (se:prevpage)) 
													(182 (se:nextpage))

													(134 (se:show-dir))
													(135 (se:loadview))
													(136 (se:quicksave))
													(137 (se:remove))
													(144 (se:save))
												)
											)
										(setf lshift nil rshift nil)
									)

									(when ctrl
										(case pressedkey
											(101 (se:lineend))
											(105 (se:mark-in))
											(111 (se:mark-out))
											(112 (se:unmark))
											(181 (se:docstart))
											((113 99) (when (se:alert "Exit") (se:cleanup) (setf se:exit t)))
											((120 98 110) (se:flush-buffer))
											((107 108) (se:flush-line))
											(104 (se:show-help))
											(114 (se:execute))
											(115 (se:search))
											((129 130 131 132 133) (se:snippet (- pressedkey 129)))

											#| SPECIAL CHARACTERS (UMLAUTS) |#
											(97 (se:insert (code-char 132)))
											(48 (se:insert (code-char 148)))
											(117 (se:insert (code-char 129)))
											(122 (se:insert (code-char 225)))
										)
										(setf ctrl nil)
									)

									(when alt
										(case pressedkey
											(120 (se:flush-line))
											(99 (se:copy))
											(105 (se:mark-in))
											(111 (se:mark-out))
											(112 (se:unmark))
											(118 (se:paste))
											(181 (se:docstart))
											(104 (se:show-help))
											(114 (se:execute))
											(115 (se:search))
											((129 130 131 132 133) (se:snippet (- pressedkey 124)))

											(32 (se:sendserial))

											#| SPECIAL CHARACTERS (UMLAUTS) |#
											(97 (se:insert (code-char 142)))
											(48 (se:insert (code-char 153)))
											(117 (se:insert (code-char 154)))
											(122 (se:insert (code-char 225)))
										)
										(setf alt nil)
									)
								)
								(case pressedkey
									(193 (setf lshift t rshift t))
									(162 (setf lshift t))
									(163 (setf rshift t))
									(165 (setf ctrl t))
									(161 (setf alt t))

									(129 (se:toggle-match))
									(130 (se:checkbr))
									(131 (se:load))
									(132 (se:viewsym))
									(133 (se:run))

									(180 (se:left))
									(183 (se:right))
									(181 (se:up))
									(182 (se:down))

									(10 (if se:help (se:show-doc) (se:enter)))
									(9 (se:tab se:numtabs))
									((8 212) (se:delete))

									(t (se:insert (code-char pressedkey)))
								)
							)
						)
						(when se:exit (fill-screen) (return t))
					)
				)
				(when (> (abs (- now se:idlestamp)) 400)
					(setf se:idlestamp (millis))
					(setf se:timestamp se:idlestamp)
					(se:show-cursor)
				)
			)
	)
	
	nil
)

;
; Helper functions
;
;
(defun constrain (value mini maxi)
	#.(format nil "(constrain value mini maxi)~%Constrain a value to interval between mini and maxi (including both).")
  (max (min value maxi) mini)
)

; Fn adopted from M5Cardputer editor version by hasn0life
; recursion eliminated, requires search-str from extensions
(defun split-string-to-list (delim str)
	#.(format nil "(split-string-to-list delimiter str)~%Split string str into list, cutting it at delimiter.")
	(unless (or (eq str nil) (not (stringp str))) 
		(let* ((start 0)
          (end (search-str delim str))
          (lst nil))
			(loop
        (if (eq end nil) 
          (return (append lst (list (subseq str start))))
				  (setq lst (append lst (list (subseq str start end)))))
        (setq start (1+ end))
        (setq end (search-str delim str start))
      )
    )
  )
)

(defun char-list-to-string (clist)
	#.(format nil "(char-list-to-string clist)~%Build string from a list of characters.")
	(let ((str "")) 
		(dolist (c clist) (setq str (concatenate 'string str c))) 
		str
	)
)

(defun to-hex-char (i)
	#.(format nil "(to-hex-char i)~%Convert number i (<= 15) to hex char.")
	(unless (equal i nil)
		(let ((i (abs i))) 
			(code-char (+ i (if (<= i 9) 48 55)) )
		)
	)
)

(defun byte-to-hexstr (i)
	#.(format nil "(byte-to-hexstr i)~%Convert number i (<= 255) to hex string.")
	(unless (equal i nil)
		(let ((i (abs i))) 
		 (concatenate 'string (string (to-hex-char (ash i -4))) (string (to-hex-char (logand i 15))) ) 
		)
	)
)

(defun word-to-hexstr (i)
	#.(format nil "(word-to-hexstr i)~%Convert number i (<= 65535) to hex string.")
	(unless (equal i nil)
		(let ((i (abs i))) 
		 (concatenate 'string (byte-to-hexstr (ash i -8)) (byte-to-hexstr (logand i 255))) 
		)
	)
)

(defun hexstr-to-int (s)
	#.(format nil "(hexstr-to-int str)~%Convert hex string to int.")
	(read-from-string (concatenate 'string "#x" s))
)

(defun remove-if (fn lis)
	#.(format nil "(remove-if fn lis)~%Remove list items when processing them with function fn evals to t.")
  (mapcan #'(lambda (item) (unless (funcall fn item) (list item))) lis)
 )

(defun remove-if-not (fn lis)
	#.(format nil "(remove-if-not fn lis)~%Remove list items when processing them with function fn evals to nil.")
  (mapcan #'(lambda (item) (when (funcall fn item) (list item))) lis)
)

(defun remove-item (ele lis)
	#.(format nil "(remove-item ele lis)~%Remove item ele from list.")
	(remove-if (lambda (item) (equal ele item)) lis)
)

(defun remove (place lis)
	#.(format nil "(remove n lis)~%Remove nth item from list.")
	(let ((newlis ()))
		(dotimes (i place)
			(push (pop lis) newlis)
		)
		(pop lis)
		(dotimes (i (length lis))
			(push (pop lis) newlis)
		)
		(reverse newlis)
	)
)

(defun assoc* (a alist test)
	#.(format nil "(assoc* key list testfn)~%Looks up a key in an association list of (key . value) pairs, and returns *all* matching pairs as a list, or nil if no pair is found.")
  (cond
   ((null alist) nil)
   ((funcall test a (caar alist)) (car alist))
   (t (assoc* a (cdr alist) test))
  )
)

(defun reverse-assoc* (a alist test)
	#.(format nil "(assoc* val list testfn)~%Looks up a value in an association list of (key . value) pairs, and returns *all* matching pairs as a list, or nil if no pair is found.")
  (cond
   ((null alist) nil)
   ((funcall test a (cdar alist)) (caar alist))
   (t (reverse-assoc* a (cdr alist) test))
  )
)

(defun get-obj (aname anum)
	#.(format nil "(get-obj aname index)~%Retrieve numbered object by providing the root name and a number.")
	(read-from-string (eval (concatenate 'string aname (string anum))))
)


#|
  Simple Package Manager v2 - 23rd February 2026
  by @apiarian. See http://www.ulisp.com/show?5KTN
|#

(defvar *pkgs* nil)

(defun package-load (file)
  "Load and eval a Lisp file from SD, tracking new symbols as a package. Returns the list of new symbols."
  (package-unload file)
  (let ((before (globals)))
    (with-sd-card (s file)
      (loop (let ((form (read s)))
        (unless form (return))
        (eval form))))
    (let ((new (mapcan (lambda (s) (unless (member s before) (list s))) (globals))))
      (setf *pkgs* (cons (cons file new) *pkgs*))
      new)))

(defun package-save (file)
  "Save a loaded package's symbols back to file on SD as defun/defvar forms. Returns file."
  (let ((pkg (assoc file *pkgs* :test #'string=)))
    (unless pkg (error "package not found: ~a" file))
    (with-sd-card (s file 2)
      (dolist (sym (cdr pkg))
        (let ((val (eval sym)))
          (if (and (consp val) (eq (car val) 'lambda))
            (pprint (cons 'defun (cons sym (cdr val))) s)
            (pprint (list 'defvar sym (list 'quote val)) s)))))
    file))

(defun package-unload (file)
  "Unbind all symbols tracked by package file and remove it from *pkgs*."
  (let ((pkg (assoc file *pkgs* :test #'string=)))
    (when pkg
      (dolist (sym (cdr pkg)) (makunbound sym))
      (setf *pkgs* (mapcan (lambda (p) (unless (string= (car p) file) (list p))) *pkgs*)))))

(defun package-add (file &rest syms)
  "Add symbols syms to package file's tracking list, creating the package if it doesn't exist. Returns syms."
  (let ((pkg (assoc file *pkgs* :test #'string=)))
    (unless pkg
      (setf *pkgs* (cons (cons file nil) *pkgs*))
      (setf pkg (car *pkgs*)))
    (dolist (sym syms)
      (unless (atom sym) (error "please quote symbols"))
      (unless (member sym (cdr pkg))
        (setf (cdr pkg) (cons sym (cdr pkg)))))
    syms))

(defun package-remove (file sym &optional unbind)
  "Remove sym from package file's tracking list. If unbind is true, also makunbound sym. Returns sym."
  (let ((pkg (assoc file *pkgs* :test #'string=)))
    (unless pkg (error "package not found: ~a" file))
    (setf (cdr pkg) (mapcan (lambda (s) (unless (eq s sym) (list s))) (cdr pkg)))
    (when unbind (makunbound sym))
    sym))

(defun package-symbols (file)
  "Return the list of symbols tracked by package file."
  (let ((pkg (assoc file *pkgs* :test #'string=)))
    (unless pkg (error "package not found: ~a" file))
    (cdr pkg)))

(defun package-list ()
  "Return a list of all loaded package filenames."
  (mapcar #'car *pkgs*))


#| uLisp Assembler |#
#| ARM Thumb Assembler for uLisp - Version 10 - 18th November 2024 |#
#| see http://www.ulisp.com/show?2YRU |#

(defun regno (sym)
  (case sym (sp 13) (lr 14) (pc 15)
    (t (read-from-string (subseq (string sym) 1)))))

(defun emit (bits &rest args)
  (let ((word 0) (shift -28))
    (mapc #'(lambda (value)
              (let ((width (logand (ash bits shift) #xf)))
                (incf shift 4)
                (unless (zerop (ash value (- width))) (error "Won't fit"))
                (setq word (logior (ash word width) value))))
          args)
    word))

(defun offset (label) (ash (- label *pc* 4) -1))

(defun $word (val)
  (append
   (unless (zerop (mod *pc* 4)) (list ($nop)))
   (list (logand val #xffff) (logand (ash val -16) #xffff))))

(defun lsl-lsr-0 (op argd argm immed5)
  (emit #x41533000 0 op immed5 (regno argm) (regno argd)))

(defun asr-0 (op argd argm immed5)
  (emit #x41533000 1 op immed5 (regno argm) (regno argd)))

(defun add-sub-1 (op argd argn argm)
  (cond
   ((numberp argm)
    (emit #x61333000 #b000111 op argm (regno argn) (regno argd)))
   ((null argm)
    (emit #x61333000 #b000110 op (regno argn) (regno argd) (regno argd)))
   (t
    (emit #x61333000 #b000110 op (regno argm) (regno argn) (regno argd)))))

(defun mov-sub-2-3 (op2 op argd immed8)
  (emit #x41380000 op2 op (regno argd) immed8))

(defun add-mov-4 (op argd argm)
  (let ((rd (regno argd))
        (rm (regno argm)))
    (cond
     ((and (>= rd 8) (>= rm 8))
      (emit #x61333000 #b010001 op #b011 (- rm 8) (- rd 8)))
     ((>= rm 8)
      (emit #x61333000 #b010001 op #b001 (- rm 8) rd))
     ((>= rd 8)
      (emit #x61333000 #b010001 op #b010 rm (- rd 8))))))

(defun reg-reg (op argd argm)
  (emit #xa3300000 op (regno argm) (regno argd)))

(defun bx-blx (op argm)
  (emit #x81430000 #b01000111 op (regno argm) 0))

(defun str-ldr (op argd arg2)
  (cond
   ((numberp arg2)
    (when (= op 0) (error "str not allowed with label"))
    (let ((arg (- (truncate (+ arg2 2) 4) (truncate *pc* 4) 1)))
      (emit #x41380000 4 1 (regno argd) (max 0 arg))))
   ((listp arg2)
    (let ((argn (first arg2))
          (immed (or (eval (second arg2)) 0)))
      (unless (zerop (mod immed 4)) (error "not multiple of 4"))
      (cond
       ((eq (regno argn) 15)
        (when (= op 0) (error "str not allowed with pc"))
        (emit #x41380000 4 1 (regno argd) (truncate immed 4)))
       ((eq (regno argn) 13)
        (emit #x41380000 9 op (regno argd) (truncate immed 4)))
       (t
        (emit #x41533000 6 op (truncate immed 4) (regno argn) (regno argd))))))
   (t (error "illegal argument"))))

(defun str-ldr-5 (op argd arg2)
  (cond
   ((listp arg2)
    (let ((argn (first arg2))
          (argm (second arg2)))
      (emit #x43333000 5 op (regno argm) (regno argn) (regno argd))))
   (t (error "illegal argument"))))

(defun add-10 (op argd immed8)
  (emit #x41380000 #b1010 op (regno argd) (truncate immed8 4)))

(defun add-sub-11 (op immed7)
  (emit #x81700000 #b11010000 op (truncate immed7 4)))

(defun push-pop (op lst)
  (let ((byte 0)
        (r 0))
    (mapc #'(lambda (x) 
              (cond
               ((and (= op 0) (eq x 'lr)) (setq r 1))
               ((and (= op 1) (eq x 'pc)) (setq r 1))
               (t (setq byte (logior byte (ash 1 (regno x))))))) lst)
    (emit #x41218000 11 op 2 r byte)))

(defun b-cond-13 (cnd label)
  (let ((soff8 (logand (offset label) #xff)))
    (emit #x44800000 13 cnd soff8)))

(defun cpside (op aif)
  (emit #xb1130000 #b10110110011 op 0 aif))

(defun $adc (argd argm)
  (reg-reg #b0100000101 argd argm))

(defun $add (argd argn &optional argm)
  (cond
   ((numberp argm)
    (cond
     ((eq (regno argn) 15)
      (add-10 0 argd argm))
     ((eq (regno argn) 13)
      (add-10 1 argd argm))
     (t (add-sub-1 0 argd argn argm))))
   ((and (numberp argn) (null argm))
    (cond
     ((eq (regno argd) 13)
      (add-sub-11 0 argn))
     (t
      (mov-sub-2-3 3 0 argd argn))))
   (t
    (cond
     ((or (>= (regno argd) 8) (>= (regno argn) 8))
      (add-mov-4 0 argd argn))
     (t
      (add-sub-1 0 argd argn argm))))))

(defun $and (argd argm)
  (reg-reg #b0100000000 argd argm))

(defun $asr (argd argm &optional arg2)
  (unless arg2 (setq arg2 argm argm argd))
  (cond
   ((numberp arg2)
    (asr-0 0 argd argm arg2))
   ((eq argd argm)
    (reg-reg #b0100000100 argd arg2))
   (t (error "First 2 registers must be the same"))))

(defun $b (label)
  (emit #x41b00000 #xe 0 (logand (offset label) #x7ff)))

(defun $bcc (label)
  (b-cond-13 3 label))

(defun $bcs (label)
  (b-cond-13 2 label))

(defun $beq (label)
  (b-cond-13 0 label))

(defun $bge (label)
  (b-cond-13 10 label))

(defun $bgt (label)
  (b-cond-13 12 label))

(defun $bhi (label)
  (b-cond-13 8 label))

(defun $bhs (label)
  (b-cond-13 2 label))

(defun $ble (label)
  (b-cond-13 13 label))

(defun $blo (label)
  (b-cond-13 3 label))

(defun $blt (label)
  (b-cond-13 11 label))

(defun $bmi (label)
  (b-cond-13 4 label))

(defun $bne (label)
  (b-cond-13 1 label))

(defun $bpl (label)
  (b-cond-13 5 label))

(defun $bic (argd argm)
  (reg-reg #b0100001110 argd argm))

(defun $bl (label)
  (list
   (emit #x5b000000 #b11110 (logand (ash (offset label) -11) #x7ff))
   (emit #x5b000000 #b11111 (logand (offset label) #x7ff))))

(defun $blx (argm)
  (bx-blx 1 argm))

(defun $bx (argm)
  (bx-blx 0 argm))

(defun $cmn (argd argm)
  (reg-reg #b0100001011 argd argm))

(defun $cmp (argd argm)
  (cond
   ((numberp argm)
    (mov-sub-2-3 2 1 argd argm))
   (t
    (reg-reg #b0100001010 argd argm))))

(defun $cpsid (aif)
  (cpside 1 aif))

(defun $cpsie (aif)
  (cpside 0 aif))
    
(defun $eor (argd argm)
  (reg-reg #b0100000001 argd argm))

(defun $ldr (argd arg2)
  (str-ldr 1 argd arg2))

(defun $ldrb (argd arg2)
  (str-ldr-5 6 argd arg2))

(defun $ldrh (argd arg2)
  (str-ldr-5 5 argd arg2))

(defun $ldrsb (argd arg2)
  (str-ldr-5 3 argd arg2))

(defun $ldrsh (argd arg2)
  (str-ldr-5 7 argd arg2))

(defun $lsl (argd argm &optional arg2)
  (unless arg2 (setq arg2 argm argm argd))
  (cond
   ((numberp arg2)
    (lsl-lsr-0 0 argd argm arg2))
   ((eq argd argm)
    (reg-reg #b0100000010 argd arg2))
   (t (error "First 2 registers must be the same"))))

(defun $lsr (argd argm &optional arg2)
  (unless arg2 (setq arg2 argm argm argd))
  (cond
   ((numberp arg2)
    (lsl-lsr-0 1 argd argm arg2))
   ((eq argd argm)
    (reg-reg #b0100000011 argd arg2))
   (t (error "First 2 registers must be the same"))))

(defun $mov (argd argm)
  (cond
   ((numberp argm)
    (mov-sub-2-3 2 0 argd argm))
   ((or (>= (regno argd) 8) (>= (regno argm) 8))
    (add-mov-4 1 argd argm))
   (t ; Synonym of LSLS Rd, Rm, #0
    (lsl-lsr-0 0 argd argm 0))))

(defun $mul (argd argm)
  (reg-reg #b0100001101 argd argm))

(defun $mvn (argd argm)
  (reg-reg #b0100001111 argd argm))

(defun $neg (argd argm)
  (reg-reg #b0100001001 argd argm))

(defun $nop () ; mov r8,r8
  (add-mov-4 1 'r8 'r8))

(defun $orr (argd argm)
  (reg-reg #b0100001100 argd argm))

(defun $push (lst)
  (push-pop 0 lst))

(defun $pop (lst)
  (push-pop 1 lst))

(defun $rev (argd argm)
  (reg-reg #b1011101000 argd argm))

(defun $rev16 (argd argm)
  (reg-reg #b1011101001 argd argm))

(defun $revsh (argd argm)
  (reg-reg #b1011101010 argd argm))

(defun $ror (argd argm)
  (reg-reg #b0100000111 argd argm))

(defun $sbc (argd argm)
  (reg-reg #b0100000110 argd argm))

(defun $str (argd arg2)
  (str-ldr 0 argd arg2))

(defun $strb (argd arg2)
  (str-ldr-5 2 argd arg2))

(defun $sub (argd argn &optional argm)
  (cond
   ((not (numberp argn))
    (add-sub-1 1 argd argn argm))
   ((eq (regno argd) 13)
      (add-sub-11 1 argn))
   (t
    (mov-sub-2-3 3 1 argd argn))))

(defun $sxtb (argd argm)
  (reg-reg #b1011001001 argd argm))

(defun $sxth (argd argm)
  (reg-reg #b1011001000 argd argm))

(defun $tst (argd argm)
  (reg-reg #b0100001000 argd argm))

(defun $uxtb (argd argm)
  (reg-reg #b1011001011 argd argm))

(defun $uxth (argd argm)
  (reg-reg #b1011001010 argd argm))


#| uLisp Compiler |#
#| Lisp compiler to ARM Thumb Assembler - Version 2a - 23rd August 2024 |#

(defun compile (name)
  (if (eq (car (eval name)) 'lambda)
      (eval (comp (cons 'defun (cons name (cdr (eval name))))))
    (error "Not a Lisp function")))

(defun comp (x &optional env)
  (cond
   ((null x) (type-code :boolean '(($mov 'r0 0))))
   ((eq x t) (type-code :boolean '(($mov 'r0 1))))
   ((symbolp x) (comp-symbol x env))
   ((atom x) (type-code :integer (list (list '$mov ''r0 x))))
   (t (let ((fn (first x)) (args (rest x)))
        (case fn
          (defun (setq *label-num* 0)
                 (setq env (mapcar #'(lambda (x y) (cons x y)) (second args) *locals*))
                 (comp-defun (first args) (second args) (cddr args) env))
          (progn (comp-progn args env))
          (if    (comp-if (first args) (second args) (third args) env))
          (setq  (comp-setq args env))
          (t     (comp-funcall fn args env)))))))

(defun mappend (fn lst)
  (apply #'append (mapcar fn lst)))

(defun type-code (type code) (cons type code))

(defun code-type (type-code) (car type-code))

(defun code (type-code) (cdr type-code))

(defun checktype (fn type check)
  (unless (or (null type) (null check) (eq type check))
    (error "Argument to '~a' must be ~a not ~a" fn check type)))

(defvar *params* '(r0 r1 r2 r3))

(defvar *locals* '(r4 r5 r6 r7))

(defvar *label-num* 0)

(defun gen-label ()
  (read-from-string (format nil "lab~d" (incf *label-num*))))

(defun comp-symbol (x env)
  (let ((reg (cdr (assoc x env))))
    (type-code nil (list (list '$mov ''r0 (list 'quote reg))))))

(defun comp-setq (args env)
  (let ((value (comp (second args) env))
        (reg (cdr (assoc (first args) env))))
    (type-code 
     (code-type value) 
     (append (code value) (list (list '$mov (list 'quote reg) ''r0))))))

(defun comp-defun (name args body env)
  (let ((used (subseq *locals* 0 (length args))))
    (append 
     (list 'defcode name args)
     (list name (list '$push (list 'quote (cons 'lr (reverse used)))))
     (apply #'append 
            (mapcar #'(lambda (x y) (list (list '$mov (list 'quote x) (list 'quote y))))
                    used *params*))
     (code (comp-progn body env))
     (list (list '$pop (list 'quote (append used (list 'pc))))))))

(defun comp-progn (exps env)
  (let* ((len (1- (length exps)))
         (nlast (subseq exps 0 len))
         (last1 (nth len exps))
         (start (mappend #'(lambda (x) (append (code (comp x env)))) nlast))
         (end (comp last1 env)))
    (type-code (code-type end) (append start (code end)))))

(defun comp-if (pred then else env)
  (let ((lab1 (gen-label))
        (lab2 (gen-label))
        (test (comp pred env)))
    (checktype 'if (car test) :boolean)
    (type-code :integer
               (append
                (code test) (list '($cmp 'r0 0) (list '$beq lab1))
                (code (comp then env)) (list (list '$b lab2) lab1)
                (code (comp else env)) (list lab2)))))

(defun comp-funcall (f args env)
  (let ((test (assoc f '((> . $bgt) (>= . $bge) (= . $beq) 
                         (<= . $ble) (< . $blt) (/= . $bne))))
        (logical (assoc f '((and . $and) (or . $orr))))
        (arith1 (assoc f '((1+ . $add) (1- . $sub))))
        (arith+- (assoc f '((+ . $add) (- . $sub))))
        (arith2 (assoc f '((* . $mul) (logand . $and) (logior . $orr) (logxor . $eor)))))
    (cond
     (test
      (let ((label (gen-label)))
        (type-code :boolean
                   (append
                    (comp-args f args 2 :integer env)
                    (list '($pop '(r1)) '($mov 'r2 1) '($cmp 'r1 'r0) 
                          (list (cdr test) label) '($mov 'r2 0) label '($mov 'r0 'r2))))))
     (logical 
      (type-code :boolean
                 (append
                  (comp-args f args 2 :boolean env)
                  (list '($pop '(r1)) (list (cdr logical) ''r0 ''r1)))))
     (arith1
      (type-code :integer 
                 (append
                  (comp-args f args 1 :integer env)
                  (list (list (cdr arith1) ''r0 1)))))
     (arith+-
      (type-code :integer 
                 (append
                  (comp-args f args 2 :integer env)
                  (list '($pop '(r1)) (list (cdr arith+-) ''r0 ''r1 ''r0)))))
     (arith2
      (type-code :integer 
                 (append
                  (comp-args f args 2 :integer env)
                  (list '($pop '(r1)) (list (cdr arith2) ''r0 ''r1)))))
     ((member f '(car cdr))
      (type-code :integer
                 (append
                  (comp-args f args 1 :integer env)
                  (if (eq f 'cdr) (list '($ldr 'r0 '(r0 4)))
                    (list '($ldr 'r0 '(r0 0)) '($ldr 'r0 '(r0 4)))))))
     (t ; function call
      (type-code :integer 
                 (append
                  (comp-args f args nil :integer env)
                  (when (> (length args) 1)
                    (append
                     (list (list '$mov (list 'quote (nth (1- (length args)) *params*)) ''r0))
                     (mappend
                      #'(lambda (x) (list (list '$pop (list 'quote (list x)))))
                      (reverse (subseq *params* 0 (1- (length args)))))))
                  (list (list '$bl f))))))))

(defun comp-args (fn args n type env)
  (unless (or (null n) (= (length args) n))
    (error "Incorrect number of arguments to '~a'" fn))
  (let ((n (length args)))
    (mappend #'(lambda (y)
                 (let ((c (comp y env)))
                   (decf n)
                   (checktype fn type (code-type c))
                   (if (zerop n) (code c) (append (code c) '(($push '(r0)))))))
             args)))

)lisplibrary";
