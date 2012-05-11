(define <)
(define *)
(define -)
(define display)

#;(define fact (lambda (x)
  (let lp ((tot 1) (x x))
    (if (< x 2)
      tot
      (lp (* tot x)(- x 1))))))

(define fact (lambda (x)
  (define lp (lambda (tot x)
    (if (< x 2)
      tot
      (lp (* tot x)(- x 1)))))
  (lp 1 x)))

(display (fact 4))

