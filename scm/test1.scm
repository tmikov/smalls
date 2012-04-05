(define x 1)

(set! x (+ x 1))
(set! x (+ x 1))

(x 10)

(define min
  (lambda (a b)
    (if (< a b) a b)))

(set! x (min x 10))
(set! x (+ x 1))

