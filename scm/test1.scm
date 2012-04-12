(define x (begin 10 20))
(define y 1)
(define + 1)
(define < 1)

(if 1 2 3)
(set! x (+ x 1))

(x 10)

(define min
  (lambda (a b)
    (if (< a b) a b)))

(set! x (min x 10))
(set! x (+ x 1))

(let ((x x))
  (begin
    (+ x 1)
    (set! x 2)
    x))

