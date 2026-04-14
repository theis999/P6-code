package dk.aau.group1.p5.chess.web;

import java.sql.Timestamp;
import java.util.List;

import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;

import dk.aau.group1.p5.chess.model.Game;
import dk.aau.group1.p5.chess.service.GameService;

@CrossOrigin(origins = "*")
@RestController
public class GameController {
    private final GameService gameService;

    public GameController(GameService gameService) {
        this.gameService = gameService;
    }

    @GetMapping("/games")
    public List<Game> getAllGames() {
        return gameService.getAll();
    }

    @GetMapping("/games/{game_id}")
    public Game getGameById(@PathVariable Long game_id) {
        return gameService.get(game_id).orElseThrow(() -> new GameNotFoundException(game_id)); 
    }

    @PostMapping("/games")
    public Game addGame(@RequestBody Game new_game) {
        if (new_game.getGamestart() == null) new_game.setGamestart(new Timestamp(System.currentTimeMillis()));
        return gameService.save(new_game);
    }

    @PatchMapping("/games/{id}")
    public Game updateGame(@PathVariable Long id, @RequestBody Game updated_game) {
        Game current_game = getGameById(id);
        current_game.setGamestate(updated_game.getGamestate()); 
        return gameService.save(current_game);
    } 
}
