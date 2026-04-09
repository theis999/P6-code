package dk.aau.group1.p5.chess.web;

import java.util.List;


import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;

import dk.aau.group1.p5.chess.model.Board;
import dk.aau.group1.p5.chess.service.BoardService;

@CrossOrigin(origins = "*")
@RestController
public class BoardController {
    private final BoardService service;

    public BoardController(BoardService boardService) {
        service = boardService;
    }

    @GetMapping("/boards")
    public List<Board> getAllBoards() {
        return service.getAll();
    }

    @PostMapping("/boards")
    public Board addBoard(@RequestBody Board new_board) {

        return service.save(new_board);
    }

    @PatchMapping("/boards/{id}")
    public Board updateBoard(@PathVariable Long id, @RequestBody Board updated_board) {
        
        return service.save(updated_board);
    }
    

}
